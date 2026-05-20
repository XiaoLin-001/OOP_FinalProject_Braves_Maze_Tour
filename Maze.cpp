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
      levelCleared(false), message("") {

    std::vector<std::vector<int>> nums;

    if (mode == GameMode::EASY) {
        // Fixed tutorial maze read from file.
        std::string filename = "maze_" + std::to_string(lv) + ".txt";
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

    // N keys on floor N.
    for (int k = 0; k < nKey && idx < empties.size(); ++k, ++idx)
        setBlock(empties[idx].first, empties[idx].second, new Key());

    // A few obstacles (more on deeper floors).
    int nObs = level + 1;
    for (int k = 0; k < nObs && idx < empties.size(); ++k, ++idx)
        setBlock(empties[idx].first, empties[idx].second, new Obstacle());

    // One pair of portals -- but only at positions that keep the Goal
    // reachable (a portal on the unique path would otherwise soft-lock).
    // Keys/obstacles never block (walkable / breakable), so we only need to
    // validate the portal pair against the maze walls.
    size_t remaining = (idx < empties.size()) ? (empties.size() - idx) : 0;
    if (remaining >= 2) {
        for (int attempt = 0; attempt < 200; ++attempt) {
            size_t i1 = idx + (size_t)(rand() % remaining);
            size_t i2 = idx + (size_t)(rand() % remaining);
            if (i1 == i2) continue;

            std::pair<int, int> p1 = empties[i1];
            std::pair<int, int> p2 = empties[i2];
            if (!reachableWithPortals(p1.first, p1.second, p2.first, p2.second))
                continue;

            Portal* A = new Portal();
            Portal* B = new Portal();
            A->setPartner(p2.first, p2.second);
            B->setPartner(p1.first, p1.second);
            setBlock(p1.first, p1.second, A);
            setBlock(p2.first, p2.second, B);
            break;
        }
        // If no safe pair is found in 200 tries, simply skip portals this floor.
    }
}

// BFS over the cells the player can stand on. The key detail: stepping onto a
// portal cell relocates you to its partner, so the portal cell is never a
// "stand" state you pass through -- you always end up on the partner instead.
bool Maze::reachableWithPortals(int ar, int ac, int br, int bc) const {
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

            // Where do we actually end up standing?
            int dr2 = yr, dc2 = yc;
            if (yr == ar && yc == ac) { dr2 = br; dc2 = bc; }   // teleport A->B
            else if (yr == br && yc == bc) { dr2 = ar; dc2 = ac; } // teleport B->A

            if (!vis[dr2][dc2]) {
                vis[dr2][dc2] = true;
                q.push(std::make_pair(dr2, dc2));
            }
        }
    }
    return vis[goalRow][goalCol];
}

// ---- moving goal ------------------------------------------------------------

void Maze::moveGoalRandom() {
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
        // Goal may only step onto plain Empty cells (never Wall/Obstacle/etc).
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

void Maze::printMaze(Player& player) {
    std::cout << "\033[2J\033[1;1H";   // clear screen + cursor home

    for (int r = 0; r < nRow; ++r) {
        for (int sr = 0; sr < 3; ++sr) {           // each block is 3 rows tall
            std::string out;
            for (int c = 0; c < nCol; ++c) {
                for (int sc = 0; sc < 3; ++sc) {   // ... and 3 cols wide
                    char ch;
                    if (r == player.getRow() && c == player.getCol()) {
                        ch = player.render(sr, sc);            // draw the hero on top
                    } else if (grid[r][c]->getVisible()) {
                        ch = grid[r][c]->render(sr, sc);
                    } else {
                        ch = (sr == 1 && sc == 1) ? '?' : ' '; // unexplored fog
                    }
                    out += ch;
                }
            }
            std::cout << out << "\n";
        }
    }

    // --- HUD ---
    std::cout << "\n";
    std::cout << "Floor: " << level << "/4    "
              << "Keys: " << player.getKeyCollected() << "/" << nKey << "    "
              << "ATK: " << player.getATK() << "    "
              << "Time: " << player.getElapsed() << "s\n";
    std::cout << "Controls:  W/A/S/D = move    E = exit\n";
    if (!message.empty())
        std::cout << ">> " << message << "\n";
}
