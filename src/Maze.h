#ifndef MAZE_H
#define MAZE_H

#include <vector>
#include <string>
#include <utility>

class Block;
class Player;

enum class GameMode { EASY, NORMAL, HARD };

class Maze {
private:
    std::vector<std::vector<Block*>> grid;
    int nRow, nCol;
    int nKey;
    int level;
    bool useMovableGoal;
    int goalRow, goalCol;
    bool levelCleared;
    bool darkMode;
    bool fullBright;
    bool replayTag;
    bool spawnMonsters;
    bool timeAttackTag;
    std::string message;

    int animTick;
    bool fxActive;
    int fxRow, fxCol;
    std::string fxColor;

    std::vector<std::pair<int, int>> toEmpty;

    void buildFromGrid(const std::vector<std::vector<int>>& nums);

    bool reachableAvoiding(const std::vector<std::pair<int, int>>& blocked,
                           const std::vector<std::pair<int, int>>& required) const;

public:
    Maze(int level, GameMode mode);
    ~Maze();

    int get_nRow() const { return nRow; }
    int get_nCol() const { return nCol; }
    int getNKey() const { return nKey; }
    int getLevel() const { return level; }
    void setDarkMode(bool v) { darkMode = v; }
    void setSpawnMonsters(bool v) { spawnMonsters = v; }
    void setFullBright(bool v) { fullBright = v; }
    void setReplayTag(bool v) { replayTag = v; }
    void setTimeAttackTag(bool v) { timeAttackTag = v; }

    bool inBounds(int r, int c) const;
    Block* at(int r, int c);
    void setBlock(int r, int c, Block* b);

    void markEmpty(int r, int c) { toEmpty.push_back(std::make_pair(r, c)); }
    void commit();

    void reveal(int r, int c);

    void setAnimTick(int t) { animTick = t; }
    void playCellFx(Player& player, int r, int c, const char* color,
                    int blinks, int onMs, int offMs);

    void printMaze(Player& player);

    bool isCleared() const { return levelCleared; }
    void setCleared(bool v) { levelCleared = v; }
    void setMessage(const std::string& m) { message = m; }

    void placeItems();
    bool usesMovableGoal() const { return useMovableGoal; }
    void moveGoalRandom(int playerRow, int playerCol);
    void setGoalPos(int r, int c) { goalRow = r; goalCol = c; }
};

#endif
