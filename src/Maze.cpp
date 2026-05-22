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

// Side length used for each floor (must be odd for the generators).
// Floor 1->9, 2->13, 3->17, 4->21 (deeper floors are bigger).
static int sizeForLevel(int level) {
    return 5 + 4 * level;
}

// ---- construction -----------------------------------------------------------

Maze::Maze(int lv, GameMode mode)
    : nRow(0), nCol(0), nKey(lv), level(lv),
      useMovableGoal(lv >= 3), goalRow(0), goalCol(0),
      levelCleared(false), darkMode(false), fullBright(false), replayTag(false),
      spawnMonsters(false), message("") {

    std::vector<std::vector<int>> nums;

    if (mode == GameMode::EASY) {
        // Fixed tutorial maze read from file.
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
    } else { // HARD
        nums = MazeGen::generateKruskal(sizeForLevel(lv));
    }

    buildFromGrid(nums);
}

// Turn the integer grid (0=Empty, 1=Wall, 2=Goal) into Block objects.
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

// ---- basic access -----------------------------------------------------------

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

// Replace every scheduled cell with a fresh Empty, keeping its visibility.
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

// ---- fog of war -------------------------------------------------------------

void Maze::reveal(int r, int c) {
    for (int i = r - 1; i <= r + 1; ++i)
        for (int j = c - 1; j <= c + 1; ++j)
            if (inBounds(i, j))
                grid[i][j]->setVisible(true);
}

// ---- random item placement --------------------------------------------------

void Maze::placeItems() {
    // Gather every empty cell except the player's start corner.
    std::vector<std::pair<int, int>> empties;
    for (int i = 0; i < nRow; ++i)
        for (int j = 0; j < nCol; ++j)
            if (grid[i][j]->getType() == BlockType::EMPTY && !(i == 1 && j == 1))
                empties.push_back(std::make_pair(i, j));

    // Fisher-Yates shuffle so positions differ on every load.
    for (size_t i = empties.size(); i > 1; --i) {
        size_t j = rand() % i;
        std::swap(empties[i - 1], empties[j]);
    }

    size_t idx = 0;

    // N keys on floor N. Remember where they went so we can guarantee they
    // stay reachable when the portals are placed.
    std::vector<std::pair<int, int>> keyCells;
    for (int k = 0; k < nKey && idx < empties.size(); ++k, ++idx) {
        setBlock(empties[idx].first, empties[idx].second, new Key());
        keyCells.push_back(empties[idx]);
    }

    // A few obstacles (more on deeper floors).
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
            int mlv = 1 + rand() % (level + 2);
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

// ---- moving goal ------------------------------------------------------------

void Maze::moveGoalRandom(int playerRow, int playerCol) {
    if (!useMovableGoal) return;

    const int dirs[4][2] = { {-1, 0}, {1, 0}, {0, -1}, {0, 1} };
    int order[4] = { 0, 1, 2, 3 };
    for (int i = 3; i > 0; --i) {            // shuffle the 4 directions
        int j = rand() % (i + 1);
        std::swap(order[i], order[j]);
    }

    for (int k = 0; k < 4; ++k) {
        int nr = goalRow + dirs[order[k]][0];
        int nc = goalCol + dirs[order[k]][1];
        // Goal may only step onto plain Empty cells (never Wall/Obstacle/etc),
        // and never onto the player.
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

// ---- rendering --------------------------------------------------------------

// A little text bar like [####------] for HP / EXP display.
static std::string makeBar(int cur, int max, int width) {
    if (max <= 0) max = 1;
    int filled = (cur * width) / max;
    if (filled < 0) filled = 0;
    if (filled > width) filled = width;
    std::string bar = "[";
    for (int i = 0; i < width; ++i) bar += (i < filled ? '#' : '-');
    bar += "]";
    return bar;
}

void Maze::printMaze(Player& player) {
    std::cout << "\033[2J\033[1;1H";   // clear screen + cursor home

    // --- Camera / viewport ---
    // Large floors are taller/wider than the terminal, so we only draw a
    // window of blocks centred on the player and scroll it as they move.
    // (Each block is 3x3 chars, so VIEW_ROWS*3 lines tall must fit the screen.)
    const int VIEW_ROWS = 11;   // blocks shown vertically
    const int VIEW_COLS = 17;   // blocks shown horizontally

    int viewR = (VIEW_ROWS < nRow) ? VIEW_ROWS : nRow;
    int viewC = (VIEW_COLS < nCol) ? VIEW_COLS : nCol;

    int top = player.getRow() - viewR / 2;
    int left = player.getCol() - viewC / 2;
    if (top < 0) top = 0;
    if (left < 0) left = 0;
    if (top > nRow - viewR) top = nRow - viewR;
    if (left > nCol - viewC) left = nCol - viewC;

    for (int r = top; r < top + viewR; ++r) {
        for (int sr = 0; sr < 3; ++sr) {           // each block is 3 rows tall
            std::string out;
            for (int c = left; c < left + viewC; ++c) {
                for (int sc = 0; sc < 3; ++sc) {   // ... and 3 cols wide
                    char ch;
                    // In dark mode only the 8 cells around the player are lit,
                    // and nothing stays remembered; otherwise use the explored
                    // (fog-of-war) flag.
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

                    if (r == player.getRow() && c == player.getCol()) {
                        ch = player.render(sr, sc);            // draw the hero on top
                    } else if (lit) {
                        ch = grid[r][c]->render(sr, sc);
                    } else {
                        ch = (sr == 1 && sc == 1) ? '?' : ' '; // unexplored / dark
                    }
                    out += ch;
                }
            }
            std::cout << out << "\n";
        }
    }

    // --- HUD ---
    std::cout << "\n";
    std::cout << "+--------------------------------------------------+\n";
    std::cout << " Floor " << level << "/4"
              << "    Keys " << player.getKeyCollected() << "/" << nKey
              << "    Time " << player.getElapsed() << "s"
              << (darkMode ? "    [Dark]" : "")
              << (fullBright ? "    [Bright]" : "")
              << (replayTag ? "    [REPLAY]" : "") << "\n";
    std::cout << " Lv " << player.getLevel() << "/" << Player::MAX_LEVEL
              << "    ATK " << player.getATK() << "\n";
    std::cout << " HP  " << makeBar(player.getHP(), player.getMaxHP(), 12)
              << " " << player.getHP() << "/" << player.getMaxHP() << "\n";
    std::cout << " EXP " << makeBar(player.getEXP(), player.getExpToNext(), 12)
              << " " << player.getEXP() << "/" << player.getExpToNext() << "\n";
    std::cout << "+--------------------------------------------------+\n";
    if (!replayTag)
        std::cout << " Controls:  W/A/S/D = move    E = exit\n";
    if (!message.empty())
        std::cout << " >> " << message << "\n";
}
