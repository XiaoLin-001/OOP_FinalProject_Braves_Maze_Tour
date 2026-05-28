#ifndef PLAYER_H
#define PLAYER_H

#include <vector>
#include <ctime>

class Maze;

enum { NAV_UP = 1001, NAV_DOWN, NAV_LEFT, NAV_RIGHT, NAV_ENTER, NAV_BACK };

char read_key();
int wait_key(int ms);
int read_nav();
void sleep_ms(int ms);

class Player {
private:
    int player_row;
    int player_col;
    int keyCollected;
    int ATK;

    int level;
    int HP, maxHP;
    int EXP, expToNext;
    int totalExp;
    int monstersDefeated;

    std::vector<std::vector<std::vector<char>>> frames;
    int frameIndex;
    time_t startTime;

public:
    Player();

    char get_direction();
    bool move(char direction, Maze& maze);
    void change_symbol();

    void setPos(int r, int c) { player_row = r; player_col = c; }
    int getRow() const { return player_row; }
    int getCol() const { return player_col; }

    int getKeyCollected() const { return keyCollected; }
    void collectKey() { keyCollected++; }

    int getATK() const { return ATK; }

    static const int MAX_LEVEL = 15;
    int getLevel() const { return level; }
    int getHP() const { return HP; }
    int getMaxHP() const { return maxHP; }
    int getEXP() const { return EXP; }
    int getExpToNext() const { return expToNext; }
    bool isDead() const { return HP <= 0; }

    int getTotalExp() const { return totalExp; }
    int getMonstersDefeated() const { return monstersDefeated; }
    void defeatMonster() { monstersDefeated++; }

    void gainEXP(int amount);
    void takeDamage(int dmg);
    void heal(int amount);

    void resetForLevel() { player_row = 1; player_col = 1; keyCollected = 0; frameIndex = 0; }

    char render(int sr, int sc) const { return frames[frameIndex][sr][sc]; }
    long getElapsed() const { return (long)(time(nullptr) - startTime); }
};

#endif
