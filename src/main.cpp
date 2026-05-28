#include "Maze.h"
#include "Player.h"
#include "Block.h"
#include "Score.h"
#include "Replay.h"
#include "Sound.h"

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
static void enableAnsiOnWindows() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(h, &mode))
        SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}
#endif

static const char* CLEAR = "\033[2J\033[1;1H";
static const char* REPLAY_DIR = "replays";

struct RunContext {
    GameMode mode;
    bool darkness;
    bool fullbright;
    bool monsters;
    bool timeAttack;
    unsigned seed;
    std::vector<int> recorded;
};

enum class RunOutcome { COMPLETED, ABORTED, DIED, TIMEOUT };

static int floorTimeLimit(int level) {
    return 45 + 30 * level;
}

static GameMode intToMode(int m) {
    if (m == 1) return GameMode::NORMAL;
    if (m == 2) return GameMode::HARD;
    return GameMode::EASY;
}

static unsigned makeSeed() {
    static unsigned counter = 0;
    return (unsigned)time(nullptr) ^ (counter++ * 2654435761u);
}

static RunOutcome runGame(RunContext& ctx, Player& player, long& clearTime) {
    srand(ctx.seed);

    for (int level = 1; level <= 4; ++level) {
        Maze maze(level, ctx.mode);
        if (maze.get_nRow() == 0) {
            std::cerr << "Failed to build floor " << level << "\n";
            clearTime = 0;
            return RunOutcome::ABORTED;
        }
        maze.setDarkMode(ctx.darkness);
        maze.setFullBright(ctx.fullbright);
        maze.setSpawnMonsters(ctx.monsters);
        maze.setTimeAttackTag(ctx.timeAttack);
        maze.placeItems();
        player.resetForLevel();
        maze.reveal(player.getRow(), player.getCol());

        time_t floorStart = time(nullptr);
        int limit = floorTimeLimit(level);

        int animTick = 0;
        const int FRAME_MS = 180;

        while (!maze.isCleared()) {
            maze.setAnimTick(animTick);
            maze.printMaze(player);

            if (ctx.timeAttack) {
                int remaining = limit - (int)(time(nullptr) - floorStart);
                if (remaining < 0) remaining = 0;
                std::cout << " TIME LEFT: " << remaining << "s / " << limit << "s\n";
                std::cout.flush();
                if (remaining <= 0) {
                    clearTime = player.getElapsed();
                    return RunOutcome::TIMEOUT;
                }
            }

            int k = wait_key(FRAME_MS);
            ++animTick;
            if (k < 0) continue;
            char dir = (char)k;

            ctx.recorded.push_back((int)(unsigned char)dir);
            if (dir == 'e' || dir == 'E') {
                clearTime = player.getElapsed();
                return RunOutcome::ABORTED;
            }
            maze.setMessage("");
            int beforeLv = player.getLevel();
            bool moved = player.move(dir, maze);
            if (player.getLevel() > beforeLv) Sound::levelUp();
            if (player.isDead()) {
                maze.printMaze(player);
                clearTime = player.getElapsed();
                return RunOutcome::DIED;
            }
            if (moved && maze.usesMovableGoal() && !maze.isCleared())
                maze.moveGoalRandom(player.getRow(), player.getCol());
        }

        if (level < 4) {
            std::cout << CLEAR;
            std::cout << "*** Floor " << level << " cleared! ***\n";
            std::cout << "Press any key for the next floor...";
            read_key();
        }
    }

    clearTime = player.getElapsed();
    return RunOutcome::COMPLETED;
}

static void showResults(GameMode mode, bool dark, bool bright, bool timeAttack,
                        long clearTime, Player& player, bool isReplay) {
    ScoreBreakdown sc = computeScore(mode, clearTime, dark, bright, timeAttack,
                                     player.getMonstersDefeated(), player.getTotalExp());
    std::string mods;
    if (dark)   mods += "Darkness ";
    if (bright) mods += "Full-bright ";
    if (timeAttack) mods += "Time ";
    if (mods.empty()) mods = "None ";

    std::cout << CLEAR;
    std::cout << "==================================================\n";
    std::cout << (isReplay ? "                R E P L A Y   E N D\n"
                           : "                   R E S U L T S\n");
    std::cout << "==================================================\n\n";
    std::cout << "  Difficulty : " << modeName(mode) << "\n";
    std::cout << "  Module     : " << mods << "\n";
    std::cout << "  Clear time : " << clearTime << "s\n";
    std::cout << "  Hero Lv    : " << player.getLevel() << "/" << Player::MAX_LEVEL
              << "    ATK " << player.getATK()
              << "    HP " << player.getHP() << "/" << player.getMaxHP() << "\n";
    std::cout << "  Monsters   : " << player.getMonstersDefeated()
              << "    Total EXP " << player.getTotalExp() << "\n\n";
    std::cout << "  Base (difficulty) : " << sc.base << "\n";
    std::cout << "  Time bonus        : " << sc.timeBonus << "\n";
    std::cout << "  Monster bonus     : " << sc.monsterBonus << "\n";
    std::cout << "  EXP bonus         : " << sc.expBonus << "\n";
    std::cout << "  Multiplier        : x" << sc.multiplier << "\n";
    std::cout << "  --------------------------------\n";
    std::cout << "  FINAL SCORE       : " << sc.total << "\n\n";
    std::cout << "==================================================\n";
    std::cout << "  Press any key to return to the menu\n";
    read_key();
}

static void renderReplayAt(const ReplayRecord& rec, int targetSteps,
                           double speedMult, bool paused, int totalSteps) {
    srand(rec.seed);
    GameMode mode = intToMode(rec.mode);
    bool dark = rec.darkness != 0;
    bool bright = rec.fullbright != 0;

    Player player;
    int applied = 0;

    for (int level = 1; level <= 4; ++level) {
        Maze maze(level, mode);
        maze.setDarkMode(dark);
        maze.setFullBright(bright);
        maze.setSpawnMonsters(rec.monsters != 0);
        maze.setTimeAttackTag(rec.timeattack != 0);
        maze.setReplayTag(true);
        maze.placeItems();
        player.resetForLevel();
        maze.reveal(player.getRow(), player.getCol());

        bool stop = false;
        while (!maze.isCleared()) {
            if (applied >= targetSteps) { stop = true; break; }
            char dir = (char)rec.keys[applied++];
            if (dir == 'e' || dir == 'E') { stop = true; break; }
            maze.setMessage("");
            bool moved = player.move(dir, maze);
            if (moved && maze.usesMovableGoal() && !maze.isCleared())
                maze.moveGoalRandom(player.getRow(), player.getCol());
        }

        if (stop || level == 4) {
            maze.printMaze(player);
            std::cout << " [REPLAY] step " << targetSteps << "/" << totalSteps
                      << "   speed " << speedMult << "x"
                      << (paused ? "   (paused)" : "") << "\n";
            std::cout << " a/d step   w/s speed   space play/pause   q quit\n";
            return;
        }
    }
}

static void watchReplay(const ReplayRecord& rec) {
    const double speeds[5] = { 0.25, 0.5, 1.0, 2.0, 4.0 };
    const int delays[5] = { 800, 400, 200, 100, 50 };

    int total = (int)rec.keys.size();
    int step = 0;
    int si = 2;
    bool paused = false;

    Sound::setMuted(true);
    while (true) {
        renderReplayAt(rec, step, speeds[si], paused, total);

        int key;
        if (!paused && step < total) {
            key = wait_key(delays[si]);
            if (key < 0) { ++step; continue; }
        } else {
            key = wait_key(-1);
        }

        char c = (char)key;
        if (c == 'q' || c == 'Q' || c == 'e' || c == 'E' || key == 27) { Sound::setMuted(false); return; }
        else if (c == ' ') paused = !paused;
        else if (c == 'a' || c == 'A') { if (step > 0) --step; paused = true; }
        else if (c == 'd' || c == 'D') { if (step < total) ++step; paused = true; }
        else if (c == 'w' || c == 'W') { if (si < 4) ++si; }
        else if (c == 's' || c == 'S') { if (si > 0) --si; }
    }
}

static void drawMenu(const std::string& title, const std::vector<std::string>& rows,
                     int sel, const std::string& hint, bool pulse) {
    const int W = 52;
    std::string bar(W, '=');
    std::ostringstream b;
    b << "\033[H";
    b << "\033[96m." << bar << ".\033[0m\033[K\n";

    int t = (int)title.size();
    if (t > W) t = W;
    int lpad = (W - t) / 2;
    int rpad = W - t - lpad;
    b << "\033[96m|\033[0m" << std::string(lpad, ' ')
      << "\033[1;95m" << title.substr(0, t) << "\033[0m"
      << std::string(rpad, ' ') << "\033[96m|\033[0m\033[K\n";
    b << "\033[96m'" << bar << "'\033[0m\033[K\n";
    b << "\033[K\n";

    for (int i = 0; i < (int)rows.size(); ++i) {
        std::string line = (i == sel ? "   > " : "     ") + rows[i];
        if ((int)line.size() > W) line = line.substr(0, W);
        line += std::string(W - (int)line.size(), ' ');
        b << " ";
        if (i == sel) b << (pulse ? "\033[1;30;107m" : "\033[1;30;106m") << line << "\033[0m";
        else          b << "\033[37m" << line << "\033[0m";
        b << "\033[K\n";
    }

    b << "\033[K\n";
    b << "\033[90m   " << hint << "\033[0m\033[K\n";
    b << "\033[J";
    std::cout << b.str();
    std::cout.flush();
}

static int runMenu(const std::string& title, const std::vector<std::string>& rows,
                   int& sel, const std::string& hint) {
    int n = (int)rows.size();
    bool pulse = false;
    while (true) {
        drawMenu(title, rows, sel, hint, pulse);
        if (pulse) { sleep_ms(55); pulse = false; continue; }
        int k = read_nav();
        if (n > 0 && k == NAV_UP)        { sel = (sel + n - 1) % n; Sound::move(); pulse = true; }
        else if (n > 0 && k == NAV_DOWN) { sel = (sel + 1) % n;     Sound::move(); pulse = true; }
        else return k;
    }
}

static GameMode chooseDifficulty(bool& chosen) {
    chosen = false;
    const GameMode modes[3] = { GameMode::EASY, GameMode::NORMAL, GameMode::HARD };
    std::vector<std::string> rows;
    rows.push_back("Easy    -  fixed tutorial maze");
    rows.push_back("Normal  -  DFS generated maze");
    rows.push_back("Hard    -  Kruskal generated maze");
    int sel = 0;
    while (true) {
        int k = runMenu("P L A Y   -   C H O O S E   M A Z E", rows, sel,
                        "Up/Down move    Enter start    Esc back");
        if (k == NAV_ENTER || k == NAV_RIGHT) { Sound::select(); chosen = true; return modes[sel]; }
        if (k == NAV_BACK || k == NAV_LEFT)   { Sound::back(); return GameMode::EASY; }
        if (k >= '1' && k <= '3') { Sound::select(); chosen = true; return modes[k - '1']; }
    }
}

static void toggleModule(int idx, bool& darkness, bool& fullbright,
                         bool& monsters, bool& timeAttack) {
    if (idx == 0)      { darkness = !darkness; if (darkness) fullbright = false; }
    else if (idx == 1) { fullbright = !fullbright; if (fullbright) darkness = false; }
    else if (idx == 2) { monsters = !monsters; }
    else if (idx == 3) { timeAttack = !timeAttack; }
}

static void modeMenu(bool& darkness, bool& fullbright, bool& monsters, bool& timeAttack) {
    int sel = 0;
    const char* labels[4] = { "Darkness", "Full-bright", "Monster", "Time Attack" };
    const char* notes[4]  = { "x1.5", "x0.5", "combat+potions", "x1.5" };
    while (true) {
        bool states[4] = { darkness, fullbright, monsters, timeAttack };
        std::vector<std::string> rows;
        for (int i = 0; i < 4; ++i) {
            std::string s = labels[i];
            while (s.size() < 13) s += ' ';
            s += states[i] ? "[ON] " : "[OFF]";
            s += "   ";
            s += notes[i];
            rows.push_back(s);
        }
        int k = runMenu("M O D E   -   E X T R A   M O D U L E S", rows, sel,
                        "Up/Down move    Enter/Space/Left/Right toggle    Esc back");
        if (k == NAV_BACK) { Sound::back(); return; }
        else if (k == NAV_ENTER || k == NAV_LEFT || k == NAV_RIGHT || k == ' ') {
            toggleModule(sel, darkness, fullbright, monsters, timeAttack);
            bool now[4] = { darkness, fullbright, monsters, timeAttack };
            Sound::toggle(now[sel]);
        } else if (k >= '1' && k <= '4') {
            int idx = k - '1';
            toggleModule(idx, darkness, fullbright, monsters, timeAttack);
            bool now[4] = { darkness, fullbright, monsters, timeAttack };
            Sound::toggle(now[idx]);
        }
    }
}

static void replayMenu() {
    int sel = 0;
    while (true) {
        std::vector<ReplayRecord> recs = loadReplays(REPLAY_DIR);
        std::reverse(recs.begin(), recs.end());

        int shown = (int)(recs.size() < 9 ? recs.size() : 9);
        if (sel >= shown) sel = shown > 0 ? shown - 1 : 0;

        std::vector<std::string> rows;
        if (shown == 0) {
            rows.push_back("(no replays saved yet)");
        } else {
            for (int i = 0; i < shown; ++i) {
                const ReplayRecord& r = recs[i];
                std::string mods = r.darkness ? "Dark" : (r.fullbright ? "Bright" : "-");
                std::string s = modeName(intToMode(r.mode));
                while (s.size() < 7) s += ' ';
                s += "  " + std::to_string(r.clearTime) + "s";
                s += "   sc " + std::to_string(r.score);
                s += "   " + mods;
                rows.push_back(s);
            }
        }

        int k = runMenu("R E P L A Y", rows, sel,
                        "Up/Down move    Enter watch    Esc back");
        if (k == NAV_BACK || k == NAV_LEFT) { Sound::back(); return; }
        else if (shown > 0 && (k == NAV_ENTER || k == NAV_RIGHT)) {
            Sound::select();
            watchReplay(recs[sel]);
        } else if (k >= '1' && k <= '9') {
            int idx = k - '1';
            if (idx < shown) { Sound::select(); watchReplay(recs[idx]); }
        }
    }
}

int main() {
#ifdef _WIN32
    enableAnsiOnWindows();
#endif
    srand((unsigned)time(nullptr));

    bool darkness = false;
    bool fullbright = false;
    bool monsters = false;
    bool timeAttack = false;

    std::cout << CLEAR;
    int sel = 0;
    while (true) {
        std::vector<std::string> items;
        items.push_back("Play");
        items.push_back("Mode");
        items.push_back("Replay");
        items.push_back("Quit");

        int k = runMenu("B R A V E ' S   M A Z E   T O U R", items, sel,
                        "Up/Down move    Enter select");

        int action = -1;
        if (k == NAV_ENTER || k == NAV_RIGHT) action = sel;
        else if (k >= '1' && k <= '4')        action = k - '1';
        else if (k == 'q' || k == 'Q')        action = 3;
        else continue;

        if (action == 0) {
            Sound::select();
            bool chosen = false;
            GameMode mode = chooseDifficulty(chosen);
            if (!chosen) continue;

            RunContext ctx;
            ctx.mode = mode;
            ctx.darkness = darkness;
            ctx.fullbright = fullbright;
            ctx.monsters = monsters;
            ctx.timeAttack = timeAttack;
            ctx.seed = makeSeed();

            Player player;
            long clearTime = 0;
            RunOutcome outcome = runGame(ctx, player, clearTime);

            if (outcome == RunOutcome::DIED || outcome == RunOutcome::TIMEOUT) {
                if (outcome == RunOutcome::TIMEOUT) Sound::timeout();
                else                                Sound::lose();
                std::cout << CLEAR;
                std::cout << "==================================================\n";
                if (outcome == RunOutcome::TIMEOUT) {
                    std::cout << "                   T I M E ' S   U P\n";
                    std::cout << "==================================================\n\n";
                    std::cout << "  You ran out of time on this floor.\n";
                } else {
                    std::cout << "                   Y O U   D I E D\n";
                    std::cout << "==================================================\n\n";
                    std::cout << "  A monster wore you down this time.\n";
                }
                std::cout << "  Hero Lv " << player.getLevel()
                          << "    survived " << clearTime << "s\n\n";
                std::cout << "  Press any key to return to the menu\n";
                read_key();
            } else if (outcome == RunOutcome::COMPLETED) {
                Sound::win();
                std::cout << CLEAR;
                std::cout << "*** Floor 4 cleared -- you conquered the maze! ***\n\n";
                std::cout << "Press Enter to see your results...";
                while (true) {
                    char k = read_key();
                    if (k == '\r' || k == '\n' || k == ' ') break;
                }

                ScoreBreakdown sc = computeScore(mode, clearTime, darkness, fullbright,
                                                 timeAttack, player.getMonstersDefeated(),
                                                 player.getTotalExp());
                ReplayRecord rr;
                rr.seed = ctx.seed;
                rr.mode = (int)mode;
                rr.darkness = darkness ? 1 : 0;
                rr.fullbright = fullbright ? 1 : 0;
                rr.monsters = monsters ? 1 : 0;
                rr.timeattack = timeAttack ? 1 : 0;
                rr.clearTime = clearTime;
                rr.score = sc.total;
                rr.keys = ctx.recorded;
                saveReplay(rr, REPLAY_DIR);

                showResults(mode, darkness, fullbright, timeAttack, clearTime, player, false);
            }
        } else if (action == 1) {
            Sound::select();
            modeMenu(darkness, fullbright, monsters, timeAttack);
        } else if (action == 2) {
            Sound::select();
            replayMenu();
        } else if (action == 3) {
            Sound::select();
            std::cout << CLEAR;
            std::cout << "Farewell, brave one!\n";
            return 0;
        }
    }
}
