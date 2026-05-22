#ifndef PLAYER_H
#define PLAYER_H

#include <vector>
#include <ctime>

class Maze;     // forward declaration

char read_key();
int wait_key(int ms);

// ----------------------------------------------------------------------------
// Player : the brave adventurer. Holds its position (in *block* coordinates,
// not pixels), how many keys it has collected, attack power, and a couple of
// animation frames so the sprite changes whenever it moves.
// ----------------------------------------------------------------------------
class Player {
private:
    int player_row;
    int player_col;
    int keyCollected;
    int ATK;                                              // attack power (grows on level up)

    // RPG stats (extra feature). HP/EXP/Level persist across floors.
    int level;
    int HP, maxHP;
    int EXP, expToNext;

    std::vector<std::vector<std::vector<char>>> frames;   // animation frames (3x3 each)
    int frameIndex;
    time_t startTime;                                     // for the on-screen game timer

public:
    Player();

    char get_direction();                  // read one WASD/E keystroke
    bool move(char direction, Maze& maze); // attempt a move; returns true if moved
    void change_symbol();                  // swap to the other animation frame

    void setPos(int r, int c) { player_row = r; player_col = c; }
    int getRow() const { return player_row; }
    int getCol() const { return player_col; }

    int getKeyCollected() const { return keyCollected; }
    void collectKey() { keyCollected++; }

    int getATK() const { return ATK; }

    // --- RPG stats ---
    static const int MAX_LEVEL = 15;
    int getLevel() const { return level; }
    int getHP() const { return HP; }
    int getMaxHP() const { return maxHP; }
    int getEXP() const { return EXP; }
    int getExpToNext() const { return expToNext; }
    bool isDead() const { return HP <= 0; }

    void gainEXP(int amount);    // add EXP, level up (and heal) when enough
    void takeDamage(int dmg);    // reduce HP, floored at 0
    void heal(int amount);       // restore HP up to maxHP

    // Reset position/keys for a new floor (stats and timer keep going).
    void resetForLevel() { player_row = 1; player_col = 1; keyCollected = 0; frameIndex = 0; }

    char render(int sr, int sc) const { return frames[frameIndex][sr][sc]; }
    long getElapsed() const { return (long)(time(nullptr) - startTime); }
};

#endif // PLAYER_H
