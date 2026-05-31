#include "Block.h"
#include "Player.h"
#include "Maze.h"
#include "Sound.h"

#include <cstdlib>
#include <string>

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

bool Block::player_touched(Player&, Maze&, int, int) {
    return false;
}

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

Wall::Wall() {
    type = BlockType::WALL;
    fill("###",
         "###",
         "###");
}

bool Wall::player_touched(Player&, Maze& maze, int, int) {
    maze.setMessage("A wall blocks your path.");
    return false;
}

Goal::Goal() {
    type = BlockType::GOAL;
    fill(" $ ",
         "$$$",
         " $ ");
}

bool Goal::player_touched(Player& player, Maze& maze, int row, int col) {
    player.setPos(row, col);
    if (player.getKeyCollected() >= maze.getNKey()) {
        maze.setCleared(true);
    } else {
        maze.setMessage("The Goal is sealed -- collect every key first!");
    }
    return true;
}

Key::Key() {
    type = BlockType::KEY;
    fill(" o ",
         "-|-",
         "   ");
}

bool Key::player_touched(Player& player, Maze& maze, int row, int col) {
    maze.playCellFx(player, row, col, "\033[7m", 2, 55, 45);
    Sound::pickup();
    player.collectKey();
    maze.setMessage("You picked up a Key!");
    maze.markEmpty(row, col);
    player.setPos(row, col);
    return true;
}

Obstacle::Obstacle() {
    type = BlockType::OBSTACLE;
    HP = 10 + (rand() % 5) * 10;
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
    return false;
}

MovableGoal::MovableGoal() {
    type = BlockType::MOVABLE_GOAL;
    fill(" $ ",
         "$@$",
         " $ ");
}

Portal::Portal() : partnerRow(1), partnerCol(1) {
    type = BlockType::PORTAL;
    fill(" @ ",
         "@O@",
         " @ ");
    symbol2.assign(3, std::vector<char>(3, ' '));
    const char* rows[3] = { " * ", "*O*", " * " };
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            symbol2[i][j] = rows[i][j];
}

char Portal::renderAt(int sr, int sc, int tick) const {
    if ((tick / 2) % 2 == 1) return symbol2[sr][sc];
    return symbol[sr][sc];
}

bool Portal::player_touched(Player& player, Maze& maze, int, int) {
    Sound::portal();
    maze.setMessage("Whoosh! A portal teleports you.");
    player.setPos(partnerRow, partnerCol);
    return true;
}

static const int MON_HP_PER_LEVEL  = 20;
static const int MON_DMG_PER_LEVEL = 5;
static const int HP_POTION_HEAL    = 40;
static const int MON_UNBEATABLE_GAP = 3;

Monster::Monster(int lv) {
    type = BlockType::MONSTER;
    level = lv;
    hp = lv * MON_HP_PER_LEVEL;
    char d = (lv >= 1 && lv <= 9) ? (char)('0' + lv) : '*';
    char mid[4] = { ' ', d, ' ', '\0' };
    fill("/M\\", mid, "\\_/");
}

bool Monster::player_touched(Player& player, Maze& maze, int row, int col) {
    maze.playCellFx(player, row, col, "\033[41m", 3, 55, 45);

    if (level - player.getLevel() >= MON_UNBEATABLE_GAP) {
        Sound::unbeatable();
        int dmg = level * MON_DMG_PER_LEVEL;
        player.takeDamage(dmg);
        maze.setMessage("Lv" + std::to_string(level) +
                        " monster is too strong to defeat (3+ levels above you)!"
                        "  -" + std::to_string(dmg) + " HP -- go around it!");
        return false;
    }

    hp -= player.getATK();
    if (hp <= 0) {
        Sound::pickup();
        int gained = 40 + level * 10;
        player.gainEXP(gained);
        player.defeatMonster();
        maze.setMessage("Defeated a Lv" + std::to_string(level) +
                        " monster!  +" + std::to_string(gained) + " EXP");
        maze.markEmpty(row, col);
        player.setPos(row, col);
        return true;
    }
    Sound::hit();
    int dmg = level * MON_DMG_PER_LEVEL;
    player.takeDamage(dmg);
    maze.setMessage("Lv" + std::to_string(level) + " monster (HP " +
                    std::to_string(hp) + ") hits back  -" + std::to_string(dmg) + " HP");
    return false;
}

ExpPotion::ExpPotion() {
    type = BlockType::EXP_POTION;
    fill(" _ ",
         "|E|",
         "\\_/");
}

bool ExpPotion::player_touched(Player& player, Maze& maze, int row, int col) {
    maze.playCellFx(player, row, col, "\033[7m", 2, 55, 45);
    Sound::potion();
    int gain = 100;
    player.gainEXP(gain);
    maze.setMessage("EXP potion!  +" + std::to_string(gain) + " EXP");
    maze.markEmpty(row, col);
    player.setPos(row, col);
    return true;
}

HpPotion::HpPotion() {
    type = BlockType::HP_POTION;
    fill(" + ",
         "|H|",
         "\\_/");
}

bool HpPotion::player_touched(Player& player, Maze& maze, int row, int col) {
    maze.playCellFx(player, row, col, "\033[7m", 2, 55, 45);
    Sound::potion();
    player.heal(HP_POTION_HEAL);
    maze.setMessage("HP potion!  +" + std::to_string(HP_POTION_HEAL) + " HP");
    maze.markEmpty(row, col);
    player.setPos(row, col);
    return true;
}
