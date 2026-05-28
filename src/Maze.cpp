#include "Maze.h"
#include "Block.h"
#include "Player.h"
#include "MazeGenerator.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <utility>
#include <queue>

static int sizeForLevel(int level) {
    return 5 + 4 * level;
}

Maze::Maze(int lv, GameMode mode)
    : nRow(0), nCol(0), nKey(lv), level(lv),
      useMovableGoal(lv >= 3), goalRow(0), goalCol(0),
      levelCleared(false), darkMode(false), fullBright(false), replayTag(false),
      spawnMonsters(false), timeAttackTag(false), message(""),
      animTick(0), fxActive(false), fxRow(-1), fxCol(-1), fxColor("") {

    std::vector<std::vector<int>> nums;

    if (mode == GameMode::EASY) {
        std::string filename = "maps/maze_" + std::to_string(lv) + ".txt";
        std::ifstream fin(filename.c_str());
        if (!fin) {
            std::cerr << "Cannot open " << filename << "\n";
            return;
        }
        std::string line;
        while (std::getline(fin, line)) {
            std::istringstream iss(line);
            std::vector<int> r;
            int v;
            while (iss >> v) r.push_back(v);
            if (!r.empty()) nums.push_back(r);
        }
    } else if (mode == GameMode::NORMAL) {
        nums = MazeGen::generateDFS(sizeForLevel(lv));
    } else {
        nums = MazeGen::generateKruskal(sizeForLevel(lv));
    }

    buildFromGrid(nums);
}

void Maze::buildFromGrid(const std::vector<std::vector<int>>& nums) {
    nRow = (int)nums.size();
    nCol = nRow ? (int)nums[0].size() : 0;
    grid.assign(nRow, std::vector<Block*>(nCol, nullptr));

    for (int i = 0; i < nRow; ++i) {
        for (int j = 0; j < nCol; ++j) {
            int v = nums[i][j];
            Block* b = nullptr;
            if (v == 1) {
                b = new Wall();
            } else if (v == 2) {
                if (useMovableGoal) b = new MovableGoal();
                else                b = new Goal();
                goalRow = i;
                goalCol = j;
            } else {
                b = new Empty();
            }
            grid[i][j] = b;
        }
    }
}

Maze::~Maze() {
    for (size_t i = 0; i < grid.size(); ++i)
        for (size_t j = 0; j < grid[i].size(); ++j)
            delete grid[i][j];
}

bool Maze::inBounds(int r, int c) const {
    return r >= 0 && r < nRow && c >= 0 && c < nCol;
}

Block* Maze::at(int r, int c) {
    return grid[r][c];
}

void Maze::setBlock(int r, int c, Block* b) {
    delete grid[r][c];
    grid[r][c] = b;
}

void Maze::commit() {
    for (size_t k = 0; k < toEmpty.size(); ++k) {
        int r = toEmpty[k].first;
        int c = toEmpty[k].second;
        bool vis = grid[r][c]->getVisible();
        delete grid[r][c];
        grid[r][c] = new Empty();
        grid[r][c]->setVisible(vis);
    }
    toEmpty.clear();
}

void Maze::reveal(int r, int c) {
    for (int i = r - 1; i <= r + 1; ++i)
        for (int j = c - 1; j <= c + 1; ++j)
            if (inBounds(i, j))
                grid[i][j]->setVisible(true);
}

void Maze::placeItems() {
    std::vector<std::pair<int, int>> empties;
    for (int i = 0; i < nRow; ++i)
        for (int j = 0; j < nCol; ++j)
            if (grid[i][j]->getType() == BlockType::EMPTY && !(i == 1 && j == 1))
                empties.push_back(std::make_pair(i, j));

    for (size_t i = empties.size(); i > 1; --i) {
        size_t j = rand() % i;
        std::swap(empties[i - 1], empties[j]);
    }

    size_t idx = 0;

    std::vector<std::pair<int, int>> keyCells;
    for (int k = 0; k < nKey && idx < empties.size(); ++k, ++idx) {
        setBlock(empties[idx].first, empties[idx].second, new Key());
        keyCells.push_back(empties[idx]);
    }

    int nObs = level + 1;
    for (int k = 0; k < nObs && idx < empties.size(); ++k, ++idx)
        setBlock(empties[idx].first, empties[idx].second, new Obstacle());

    std::vector<std::pair<int, int>> required = keyCells;
    required.push_back(std::make_pair(goalRow, goalCol));

    std::vector<std::pair<int, int>> blocked;

    if (spawnMonsters) {
        std::vector<std::pair<int, int>> mcand;
        for (int i = 0; i < nRow; ++i)
            for (int j = 0; j < nCol; ++j)
                if (grid[i][j]->getType() == BlockType::EMPTY && !(i == 1 && j == 1))
                    mcand.push_back(std::make_pair(i, j));
        for (size_t i = mcand.size(); i > 1; --i) {
            size_t j = rand() % i;
            std::swap(mcand[i - 1], mcand[j]);
        }

        int want = level + 1;
        int done = 0;
        for (size_t i = 0; i < mcand.size() && done < want; ++i) {
            std::vector<std::pair<int, int>> trial = blocked;
            trial.push_back(mcand[i]);
            if (!reachableAvoiding(trial, required)) continue;
            int mlv = 1 + rand() % (2 * level);
            setBlock(mcand[i].first, mcand[i].second, new Monster(mlv));
            blocked.push_back(mcand[i]);
            ++done;
        }
    }

    std::vector<std::pair<int, int>> cand;
    for (int i = 0; i < nRow; ++i)
        for (int j = 0; j < nCol; ++j)
            if (grid[i][j]->getType() == BlockType::EMPTY && !(i == 1 && j == 1))
                cand.push_back(std::make_pair(i, j));

    std::vector<std::pair<int, int>> safe;
    for (size_t i = 0; i < cand.size(); ++i) {
        std::vector<std::pair<int, int>> trial = blocked;
        trial.push_back(cand[i]);
        if (reachableAvoiding(trial, required)) safe.push_back(cand[i]);
    }

    for (size_t i = safe.size(); i > 1; --i) {
        size_t j = rand() % i;
        std::swap(safe[i - 1], safe[j]);
    }

    bool placed = false;
    for (size_t a = 0; a < safe.size() && !placed; ++a) {
        for (size_t b = a + 1; b < safe.size() && !placed; ++b) {
            std::vector<std::pair<int, int>> trial = blocked;
            trial.push_back(safe[a]);
            trial.push_back(safe[b]);
            if (!reachableAvoiding(trial, required)) continue;
            Portal* A = new Portal();
            Portal* B = new Portal();
            A->setPartner(safe[b].first, safe[b].second);
            B->setPartner(safe[a].first, safe[a].second);
            setBlock(safe[a].first, safe[a].second, A);
            setBlock(safe[b].first, safe[b].second, B);
            placed = true;
        }
    }

    if (spawnMonsters) {
        std::vector<std::pair<int, int>> pcand;
        for (int i = 0; i < nRow; ++i)
            for (int j = 0; j < nCol; ++j)
                if (grid[i][j]->getType() == BlockType::EMPTY && !(i == 1 && j == 1))
                    pcand.push_back(std::make_pair(i, j));
        for (size_t i = pcand.size(); i > 1; --i) {
            size_t j = rand() % i;
            std::swap(pcand[i - 1], pcand[j]);
        }
        size_t pi = 0;
        int wantExp = level + 1;
        for (int k = 0; k < wantExp && pi < pcand.size(); ++k, ++pi)
            setBlock(pcand[pi].first, pcand[pi].second, new ExpPotion());
        int wantHp = level;
        for (int k = 0; k < wantHp && pi < pcand.size(); ++k, ++pi)
            setBlock(pcand[pi].first, pcand[pi].second, new HpPotion());
    }
}

bool Maze::reachableAvoiding(const std::vector<std::pair<int, int>>& blocked,
                             const std::vector<std::pair<int, int>>& required) const {
    std::vector<std::vector<bool>> block(nRow, std::vector<bool>(nCol, false));
    for (size_t i = 0; i < blocked.size(); ++i)
        block[blocked[i].first][blocked[i].second] = true;

    std::vector<std::vector<bool>> vis(nRow, std::vector<bool>(nCol, false));
    std::queue<std::pair<int, int>> q;
    vis[1][1] = true;
    q.push(std::make_pair(1, 1));

    const int dr[4] = { -1, 1, 0, 0 };
    const int dc[4] = { 0, 0, -1, 1 };

    while (!q.empty()) {
        int cr = q.front().first;
        int cc = q.front().second;
        q.pop();

        for (int k = 0; k < 4; ++k) {
            int yr = cr + dr[k], yc = cc + dc[k];
            if (!inBounds(yr, yc)) continue;
            if (grid[yr][yc]->getType() == BlockType::WALL) continue;
            if (block[yr][yc]) continue;

            if (!vis[yr][yc]) {
                vis[yr][yc] = true;
                q.push(std::make_pair(yr, yc));
            }
        }
    }

    for (size_t i = 0; i < required.size(); ++i)
        if (!vis[required[i].first][required[i].second])
            return false;
    return true;
}

void Maze::moveGoalRandom(int playerRow, int playerCol) {
    if (!useMovableGoal) return;

    const int dirs[4][2] = { {-1, 0}, {1, 0}, {0, -1}, {0, 1} };
    int order[4] = { 0, 1, 2, 3 };
    for (int i = 3; i > 0; --i) {
        int j = rand() % (i + 1);
        std::swap(order[i], order[j]);
    }

    for (int k = 0; k < 4; ++k) {
        int nr = goalRow + dirs[order[k]][0];
        int nc = goalCol + dirs[order[k]][1];
        if (nr == playerRow && nc == playerCol) continue;
        if (inBounds(nr, nc) && grid[nr][nc]->getType() == BlockType::EMPTY) {
            bool gVis = grid[goalRow][goalCol]->getVisible();
            bool tVis = grid[nr][nc]->getVisible();

            delete grid[goalRow][goalCol];
            grid[goalRow][goalCol] = new Empty();
            grid[goalRow][goalCol]->setVisible(gVis);

            delete grid[nr][nc];
            MovableGoal* mg = new MovableGoal();
            mg->setVisible(tVis);
            grid[nr][nc] = mg;

            goalRow = nr;
            goalCol = nc;
            return;
        }
    }
}

static const char* RESET = "\033[0m";

static const char* colorFor(BlockType t) {
    switch (t) {
        case BlockType::WALL:         return "\033[90m";
        case BlockType::GOAL:
        case BlockType::MOVABLE_GOAL: return "\033[93m";
        case BlockType::KEY:          return "\033[1;93m";
        case BlockType::OBSTACLE:     return "\033[31m";
        case BlockType::PORTAL:       return "\033[96m";
        case BlockType::MONSTER:      return "\033[91m";
        case BlockType::EXP_POTION:   return "\033[92m";
        case BlockType::HP_POTION:    return "\033[95m";
        default:                      return "";
    }
}

static std::string makeBar(int cur, int max, int width, const char* color) {
    if (max <= 0) max = 1;
    int filled = (cur * width) / max;
    if (filled < 0) filled = 0;
    if (filled > width) filled = width;
    std::string bar = "[";
    bar += color;
    for (int i = 0; i < width; ++i) bar += (i < filled ? '#' : '-');
    bar += RESET;
    bar += "]";
    return bar;
}

void Maze::printMaze(Player& player) {
    std::ostringstream buf;
    buf << "\033[H";

    const char* EOL = "\033[K\n";

    const int VIEW_ROWS = 11;
    const int VIEW_COLS = 17;

    int viewR = (VIEW_ROWS < nRow) ? VIEW_ROWS : nRow;
    int viewC = (VIEW_COLS < nCol) ? VIEW_COLS : nCol;

    int top = player.getRow() - viewR / 2;
    int left = player.getCol() - viewC / 2;
    if (top < 0) top = 0;
    if (left < 0) left = 0;
    if (top > nRow - viewR) top = nRow - viewR;
    if (left > nCol - viewC) left = nCol - viewC;

    for (int r = top; r < top + viewR; ++r) {
        for (int sr = 0; sr < 3; ++sr) {
            std::string out;
            for (int c = left; c < left + viewC; ++c) {
                bool lit;
                if (fullBright) {
                    lit = true;
                } else if (darkMode) {
                    int ddr = r - player.getRow(); if (ddr < 0) ddr = -ddr;
                    int ddc = c - player.getCol(); if (ddc < 0) ddc = -ddc;
                    lit = (ddr <= 1 && ddc <= 1);
                } else {
                    lit = grid[r][c]->getVisible();
                }

                bool isFx = (fxActive && r == fxRow && c == fxCol);
                if (isFx) lit = true;

                bool isPlayer = (r == player.getRow() && c == player.getCol());
                const char* col;
                if (isFx)          col = fxColor.c_str();
                else if (isPlayer) col = "\033[1;92m";
                else if (!lit)     col = "\033[90m";
                else               col = colorFor(grid[r][c]->getType());

                out += col;
                for (int sc = 0; sc < 3; ++sc) {
                    char ch;
                    if (isPlayer) {
                        ch = player.render(sr, sc);
                    } else if (lit) {
                        ch = grid[r][c]->renderAt(sr, sc, animTick);
                    } else {
                        ch = (sr == 1 && sc == 1) ? '?' : ' ';
                    }
                    out += ch;
                }
                out += RESET;
            }
            buf << out << EOL;
        }
    }

    buf << EOL;
    buf << "+--------------------------------------------------+" << EOL;
    buf << " Floor " << level << "/4"
        << "    Keys " << player.getKeyCollected() << "/" << nKey
        << "    Time " << player.getElapsed() << "s"
        << (darkMode ? "    [Dark]" : "")
        << (fullBright ? "    [Bright]" : "")
        << (timeAttackTag ? "    [Time]" : "")
        << (replayTag ? "    [REPLAY]" : "") << EOL;
    buf << " Lv " << player.getLevel() << "/" << Player::MAX_LEVEL
        << "    ATK " << player.getATK() << EOL;
    buf << " HP  " << makeBar(player.getHP(), player.getMaxHP(), 12, "\033[91m")
        << " " << player.getHP() << "/" << player.getMaxHP() << EOL;
    buf << " EXP " << makeBar(player.getEXP(), player.getExpToNext(), 12, "\033[92m")
        << " " << player.getEXP() << "/" << player.getExpToNext() << EOL;
    buf << "+--------------------------------------------------+" << EOL;
    if (!replayTag)
        buf << " Controls:  W/A/S/D = move    E = exit" << EOL;
    if (!message.empty())
        buf << " >> " << message << EOL;

    buf << "\033[J";

    std::cout << buf.str();
    std::cout.flush();
}

void Maze::playCellFx(Player& player, int r, int c, const char* color,
                      int blinks, int onMs, int offMs) {
    if (replayTag) return;
    fxRow = r;
    fxCol = c;
    fxColor = color;
    for (int i = 0; i < blinks; ++i) {
        fxActive = true;
        printMaze(player);
        sleep_ms(onMs);
        fxActive = false;
        printMaze(player);
        sleep_ms(offMs);
    }
    fxActive = false;
}
