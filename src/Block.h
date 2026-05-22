#ifndef BLOCK_H
#define BLOCK_H

#include <vector>

// Type tag so the Maze / renderer can ask a Block what it is
// without a dynamic_cast everywhere.
enum class BlockType {
    EMPTY, WALL, GOAL, KEY, OBSTACLE, PORTAL, MOVABLE_GOAL, MONSTER
};

class Player;   // forward declaration (see Player.h)
class Maze;     // forward declaration (see Maze.h)

// ----------------------------------------------------------------------------
// Block : base class for every cell in the maze.
// Each Block carries a 3x3 grid of characters (its "symbol") that is what gets
// drawn on the terminal, plus a visibility flag used for the fog-of-war.
// ----------------------------------------------------------------------------
class Block {
protected:
    std::vector<std::vector<char>> symbol;  // 3x3 picture of this block
    bool isVisible;                         // has the player explored it yet?
    BlockType type;

    // Convenience helper: fill the 3x3 symbol from three 3-char rows.
    void fill(const char* row0, const char* row1, const char* row2);

public:
    Block();
    virtual ~Block();

    // Called when the player tries to step onto this block.
    // Returns true if the player ends up changing position because of it.
    // Each subclass decides the collision behaviour.
    virtual bool player_touched(Player& player, Maze& maze, int row, int col);

    char render(int sr, int sc) const { return symbol[sr][sc]; }

    bool getVisible() const { return isVisible; }
    void setVisible(bool v) { isVisible = v; }
    BlockType getType() const { return type; }
};

// --- Provided basic elements --------------------------------------------------

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

// --- Student-created elements -------------------------------------------------

// Key: must collect all of them before the Goal opens.
class Key : public Block {
public:
    Key();
    bool player_touched(Player& player, Maze& maze, int row, int col) override;
};

// Obstacle: blocks the path until the player breaks it with ATK.
class Obstacle : public Block {
private:
    int HP;     // 10 ~ 50 at creation
public:
    Obstacle();
    int getHP() const { return HP; }
    bool player_touched(Player& player, Maze& maze, int row, int col) override;
};

// MovableGoal: a Goal that wanders one step each turn (floors 3 & 4).
class MovableGoal : public Goal {
public:
    MovableGoal();
};

// Portal: paired teleporter. Stepping on one drops you on its partner.
class Portal : public Block {
private:
    int partnerRow, partnerCol;
public:
    Portal();
    void setPartner(int r, int c) { partnerRow = r; partnerCol = c; }
    bool player_touched(Player& player, Maze& maze, int row, int col) override;
};

class Monster : public Block {
private:
    int level;
public:
    Monster(int lv);
    int getLevel() const { return level; }
    bool player_touched(Player& player, Maze& maze, int row, int col) override;
};

#endif // BLOCK_H
