#include "Block.h"
#include "Player.h"
#include "Maze.h"

#include <cstdlib>
#include <string>

// ---- Block (base) -----------------------------------------------------------

Block::Block() : isVisible(false), type(BlockType::EMPTY) {
    symbol.assign(3, std::vector<char>(3, ' '));
}

Block::~Block() {}

void Block::fill(const char* r0, const char* r1, const char* r2) {
    symbol.assign(3, std::vector<char>(3, ' '));
    const char* rows[3] = { r0, r1, r2 };
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            symbol[i][j] = rows[i][j];
}

// Default: an unknown block blocks movement (should never be reached).
bool Block::player_touched(Player&, Maze&, int, int) {
    return false;
}

// ---- Empty ------------------------------------------------------------------

Empty::Empty() {
    type = BlockType::EMPTY;
    fill("   ",
         "   ",
         "   ");
}

bool Empty::player_touched(Player& player, Maze&, int row, int col) {
    player.setPos(row, col);
    return true;
}

// ---- Wall -------------------------------------------------------------------

Wall::Wall() {
    type = BlockType::WALL;
    fill("###",
         "###",
         "###");
}

bool Wall::player_touched(Player&, Maze& maze, int, int) {
    maze.setMessage("A wall blocks your path.");
    return false;   // invalid move
}

// ---- Goal -------------------------------------------------------------------

Goal::Goal() {
    type = BlockType::GOAL;
    fill(" $ ",
         "$$$",
         " $ ");
}

bool Goal::player_touched(Player& player, Maze& maze, int row, int col) {
    if (player.getKeyCollected() >= maze.getNKey()) {
        maze.setCleared(true);
        player.setPos(row, col);
        return true;
    }
    maze.setMessage("Collect every key before entering the Goal!");
    return false;
}

// ---- Key --------------------------------------------------------------------

Key::Key() {
    type = BlockType::KEY;
    fill(" o ",
         "-|-",
         "   ");
}

bool Key::player_touched(Player& player, Maze& maze, int row, int col) {
    player.collectKey();
    maze.setMessage("You picked up a Key!");
    maze.markEmpty(row, col);   // the key is gone after this
    player.setPos(row, col);
    return true;
}

// ---- Obstacle ---------------------------------------------------------------

Obstacle::Obstacle() {
    type = BlockType::OBSTACLE;
    HP = 10 + (rand() % 5) * 10;    // 10, 20, 30, 40 or 50
    fill("/X\\",
         "XXX",
         "\\X/");
}

bool Obstacle::player_touched(Player& player, Maze& maze, int row, int col) {
    HP -= player.getATK();
    if (HP <= 0) {
        maze.setMessage("The obstacle shatters!");
        maze.markEmpty(row, col);
    } else {
        maze.setMessage("You strike the obstacle. HP left: " + std::to_string(HP));
    }
    return false;   // never move onto the obstacle the same turn it is hit
}

// ---- MovableGoal ------------------------------------------------------------

MovableGoal::MovableGoal() {
    type = BlockType::MOVABLE_GOAL;
    fill(" $ ",
         "$@$",
         " $ ");
}

// ---- Portal -----------------------------------------------------------------

Portal::Portal() : partnerRow(1), partnerCol(1) {
    type = BlockType::PORTAL;
    fill(" @ ",
         "@O@",
         " @ ");
}

bool Portal::player_touched(Player& player, Maze& maze, int, int) {
    maze.setMessage("Whoosh! A portal teleports you.");
    player.setPos(partnerRow, partnerCol);
    return true;
}
