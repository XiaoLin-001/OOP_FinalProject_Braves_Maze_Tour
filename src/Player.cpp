#include "Player.h"
#include "Maze.h"
#include "Block.h"

#include <iostream>

#ifdef _WIN32
  #include <conio.h>
  #include <windows.h>
  static char getch_raw() {
      return (char)_getch();
  }
  int wait_key(int ms) {
      if (ms < 0) {
          while (!_kbhit()) Sleep(10);
          return _getch();
      }
      int waited = 0;
      while (waited < ms) {
          if (_kbhit()) return _getch();
          Sleep(10);
          waited += 10;
      }
      return _kbhit() ? _getch() : -1;
  }
#else
  #include <termios.h>
  #include <unistd.h>
  #include <sys/select.h>
  static char getch_raw() {
      struct termios oldt, newt;
      tcgetattr(STDIN_FILENO, &oldt);
      newt = oldt;
      newt.c_lflag &= ~(ICANON | ECHO);
      tcsetattr(STDIN_FILENO, TCSANOW, &newt);

      char ch = 0;
      ssize_t n = read(STDIN_FILENO, &ch, 1);

      tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
      if (n <= 0) return 'E';
      return ch;
  }
  int wait_key(int ms) {
      struct termios oldt, newt;
      tcgetattr(STDIN_FILENO, &oldt);
      newt = oldt;
      newt.c_lflag &= ~(ICANON | ECHO);
      tcsetattr(STDIN_FILENO, TCSANOW, &newt);

      fd_set set;
      FD_ZERO(&set);
      FD_SET(STDIN_FILENO, &set);
      struct timeval tv;
      struct timeval* ptv = nullptr;
      if (ms >= 0) {
          tv.tv_sec = ms / 1000;
          tv.tv_usec = (ms % 1000) * 1000;
          ptv = &tv;
      }

      int result = -1;
      if (select(STDIN_FILENO + 1, &set, nullptr, nullptr, ptv) > 0) {
          char c;
          if (read(STDIN_FILENO, &c, 1) > 0) result = (unsigned char)c;
      }
      tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
      return result;
  }
#endif

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
    : player_row(1), player_col(1), keyCollected(0), ATK(10),
      level(1), HP(100), maxHP(100), EXP(0), expToNext(100), frameIndex(0) {
    // Two little stick-figure frames; the legs/arms flip when walking.
    frames.push_back(makeFrame(" o ",
                               "/|\\",
                               "/ \\"));
    frames.push_back(makeFrame(" o ",
                               "\\|/",
                               " | "));
    startTime = time(nullptr);
}

char read_key() {
    return getch_raw();
}

char Player::get_direction() {
    return read_key();
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

void Player::gainEXP(int amount) {
    if (level >= MAX_LEVEL) return;
    EXP += amount;
    while (level < MAX_LEVEL && EXP >= expToNext) {
        EXP -= expToNext;
        level++;
        maxHP += 20;
        HP = maxHP;          // full heal on level up
        ATK += 5;
        expToNext += 50;     // each level needs a bit more
    }
    if (level >= MAX_LEVEL) EXP = 0;   // capped
}

void Player::takeDamage(int dmg) {
    HP -= dmg;
    if (HP < 0) HP = 0;
}

void Player::heal(int amount) {
    HP += amount;
    if (HP > maxHP) HP = maxHP;
}
