#include "Player.h"
#include "Maze.h"
#include "Block.h"

#include <iostream>

// Build one 3x3 animation frame from three 3-char rows.
static std::vector<std::vector<char>> makeFrame(const char* a, const char* b, const char* c) {
    std::vector<std::vector<char>> f(3, std::vector<char>(3, ' '));
    const char* rows[3] = { a, b, c };
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            f[i][j] = rows[i][j];
    return f;
}

Player::Player()
    : player_row(1), player_col(1), keyCollected(0), ATK(10), frameIndex(0) {
    // Two little stick-figure frames; the legs/arms flip when walking.
    frames.push_back(makeFrame(" o ",
                               "/|\\",
                               "/ \\"));
    frames.push_back(makeFrame(" o ",
                               "\\|/",
                               " | "));
    startTime = time(nullptr);
}

char Player::get_direction() {
    char ch;
    std::cin >> ch;
    return ch;
}

void Player::change_symbol() {
    frameIndex = 1 - frameIndex;
}

bool Player::move(char direction, Maze& maze) {
    int dr = 0, dc = 0;
    switch (direction) {
        case 'w': case 'W': dr = -1; break;
        case 's': case 'S': dr =  1; break;
        case 'a': case 'A': dc = -1; break;
        case 'd': case 'D': dc =  1; break;
        default: return false;
    }

    int tr = player_row + dr;
    int tc = player_col + dc;
    if (!maze.inBounds(tr, tc))
        return false;

    // Let the target block decide what happens (polymorphic collision).
    Block* target = maze.at(tr, tc);
    bool moved = target->player_touched(*this, maze, tr, tc);

    if (moved) {
        change_symbol();                       // sprite changes on every move
        maze.reveal(player_row, player_col);   // light up the new surroundings
    }
    maze.commit();   // turn any collected keys / broken obstacles into Empty
    return moved;
}
