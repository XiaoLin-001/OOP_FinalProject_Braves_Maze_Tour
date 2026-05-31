#ifndef BLOCK_H
#define BLOCK_H

#include <vector>

enum class BlockType {
    EMPTY, WALL, GOAL, KEY, OBSTACLE, PORTAL, MOVABLE_GOAL, MONSTER,
    EXP_POTION, HP_POTION
};

class Player;
class Maze;

class Block {
protected:
    std::vector<std::vector<char>> symbol;
    bool isVisible;
    BlockType type;

    void fill(const char* row0, const char* row1, const char* row2);

public:
    Block();
    virtual ~Block();

    virtual bool player_touched(Player& player, Maze& maze, int row, int col);

    char render(int sr, int sc) const { return symbol[sr][sc]; }
    virtual char renderAt(int sr, int sc, int tick) const { (void)tick; return symbol[sr][sc]; }

    bool getVisible() const { return isVisible; }
    void setVisible(bool v) { isVisible = v; }
    BlockType getType() const { return type; }
};

class Empty : public Block {
public:
    Empty();
    bool player_touched(Player& player, Maze& maze, int row, int col) override;
};

class Wall : public Block {
public:
    Wall();
    bool player_touched(Player& player, Maze& maze, int row, int col) override;
};

class Goal : public Block {
public:
    Goal();
    bool player_touched(Player& player, Maze& maze, int row, int col) override;
};

class Key : public Block {
public:
    Key();
    bool player_touched(Player& player, Maze& maze, int row, int col) override;
};

class Obstacle : public Block {
private:
    int HP;
public:
    Obstacle();
    int getHP() const { return HP; }
    bool player_touched(Player& player, Maze& maze, int row, int col) override;
};

class MovableGoal : public Goal {
public:
    MovableGoal();
};

class Portal : public Block {
private:
    int partnerRow, partnerCol;
    std::vector<std::vector<char>> symbol2;
public:
    Portal();
    void setPartner(int r, int c) { partnerRow = r; partnerCol = c; }
    char renderAt(int sr, int sc, int tick) const override;
    bool player_touched(Player& player, Maze& maze, int row, int col) override;
};

class Monster : public Block {
private:
    int level;
    int hp;
public:
    Monster(int lv);
    int getLevel() const { return level; }
    bool player_touched(Player& player, Maze& maze, int row, int col) override;
};

class ExpPotion : public Block {
public:
    ExpPotion();
    bool player_touched(Player& player, Maze& maze, int row, int col) override;
};

class HpPotion : public Block {
public:
    HpPotion();
    bool player_touched(Player& player, Maze& maze, int row, int col) override;
};

#endif
