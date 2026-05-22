#include "Maze.h"
#include "Player.h"
#include "Block.h"
#include "Score.h"
#include "Replay.h"

#include <iostream>
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
    unsigned seed;
    std::vector<int> recorded;
};

enum class RunOutcome { COMPLETED, ABORTED, DIED };

static bool isBack(char k) {
    return k == 'b' || k == 'B' || k == 27;
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
        maze.placeItems();
        player.resetForLevel();
        maze.reveal(player.getRow(), player.getCol());

        while (!maze.isCleared()) {
            maze.printMaze(player);
            char dir = read_key();
            ctx.recorded.push_back((int)(unsigned char)dir);
            if (dir == 'e' || dir == 'E') {
                clearTime = player.getElapsed();
                return RunOutcome::ABORTED;
            }
            maze.setMessage("");
            bool moved = player.move(dir, maze);
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

static void showResults(GameMode mode, bool dark, bool bright,
                        long clearTime, Player& player, bool isReplay) {
    ScoreBreakdown sc = computeScore(mode, clearTime, dark, bright);
    std::string mods = dark ? "Darkness (x1.5)"
                            : (bright ? "Full-bright (x0.5)" : "None (x1.0)");

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
              << "    HP " << player.getHP() << "/" << player.getMaxHP() << "\n\n";
    std::cout << "  Base (difficulty) : " << sc.base << "\n";
    std::cout << "  Time bonus        : " << sc.timeBonus << "\n";
    std::cout << "  Multiplier        : x" << sc.multiplier << "\n";
    std::cout << "  --------------------------------\n";
    std::cout << "  FINAL SCORE       : " << sc.total << "\n\n";
    std::cout << "==================================================\n";
    std::cout << "  Press any key to return to the menu\n";
    read_key();
}

static void renderReplayAt(const ReplayRecord& rec, int targetSteps,
                           int delayMs, bool paused, int totalSteps) {
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
                      << "   speed " << delayMs << "ms"
                      << (paused ? "   (paused)" : "") << "\n";
            std::cout << " a/d step   w/s speed   space play/pause   q quit\n";
            return;
        }
    }
}

static void watchReplay(const ReplayRecord& rec) {
    int total = (int)rec.keys.size();
    int step = 0;
    int delayMs = 200;
    bool paused = false;

    while (true) {
        renderReplayAt(rec, step, delayMs, paused, total);

        int key;
        if (!paused && step < total) {
            key = wait_key(delayMs);
            if (key < 0) { ++step; continue; }
        } else {
            key = wait_key(-1);
        }

        char c = (char)key;
        if (c == 'q' || c == 'Q' || c == 'e' || c == 'E' || key == 27) return;
        else if (c == ' ') paused = !paused;
        else if (c == 'a' || c == 'A') { if (step > 0) --step; paused = true; }
        else if (c == 'd' || c == 'D') { if (step < total) ++step; paused = true; }
        else if (c == 'w' || c == 'W') { delayMs -= 50; if (delayMs < 50) delayMs = 50; }
        else if (c == 's' || c == 'S') { delayMs += 50; if (delayMs > 1000) delayMs = 1000; }
    }
}

static GameMode chooseDifficulty(bool& chosen) {
    chosen = false;
    while (true) {
        std::cout << CLEAR;
        std::cout << "==============================\n";
        std::cout << "        Play - Difficulty\n";
        std::cout << "==============================\n\n";
        std::cout << "  [1] Easy   - fixed tutorial maze\n";
        std::cout << "  [2] Normal - DFS generated maze\n";
        std::cout << "  [3] Hard   - Kruskal generated maze\n\n";
        std::cout << "  [B] Back\n\n";
        std::cout << "Select...";

        char k = read_key();
        if (k == '1') { chosen = true; return GameMode::EASY; }
        if (k == '2') { chosen = true; return GameMode::NORMAL; }
        if (k == '3') { chosen = true; return GameMode::HARD; }
        if (isBack(k)) return GameMode::EASY;
    }
}

static void modeMenu(bool& darkness, bool& fullbright, bool& monsters) {
    while (true) {
        std::cout << CLEAR;
        std::cout << "==============================\n";
        std::cout << "       Mode - Extra Modules\n";
        std::cout << "==============================\n\n";
        std::cout << "  [1] Darkness Mode    : " << (darkness ? "ON " : "OFF")
                  << "   (only 8 cells lit, no memory, score x1.5)\n";
        std::cout << "  [2] Full-bright Mode : " << (fullbright ? "ON " : "OFF")
                  << "   (whole map visible, score x0.5)\n";
        std::cout << "  [3] Monster          : " << (monsters ? "ON " : "OFF")
                  << "   (Lv <= you: defeat for EXP;  Lv > you: death)\n\n";
        std::cout << "  [B] Back\n\n";
        std::cout << "Press 1 / 2 / 3 to toggle...";

        char k = read_key();
        if (k == '1') { darkness = !darkness; if (darkness) fullbright = false; }
        else if (k == '2') { fullbright = !fullbright; if (fullbright) darkness = false; }
        else if (k == '3') { monsters = !monsters; }
        else if (isBack(k)) return;
    }
}

static void replayMenu() {
    while (true) {
        std::vector<ReplayRecord> recs = loadReplays(REPLAY_DIR);
        std::reverse(recs.begin(), recs.end());

        std::cout << CLEAR;
        std::cout << "==============================\n";
        std::cout << "            Replay\n";
        std::cout << "==============================\n\n";

        if (recs.empty()) {
            std::cout << "  (no replays saved yet)\n\n";
        } else {
            size_t shown = recs.size() < 9 ? recs.size() : 9;
            for (size_t i = 0; i < shown; ++i) {
                const ReplayRecord& r = recs[i];
                std::string mods = r.darkness ? "Dark" : (r.fullbright ? "Bright" : "-");
                std::cout << "  [" << (i + 1) << "] " << r.name << "\n"
                          << "       " << modeName(intToMode(r.mode))
                          << "   time " << r.clearTime << "s"
                          << "   score " << r.score
                          << "   mod " << mods << "\n";
            }
            std::cout << "\n";
        }

        std::cout << "  [B] Back\n\n";
        std::cout << "Select a number to watch...";

        char k = read_key();
        if (isBack(k)) return;
        if (k >= '1' && k <= '9') {
            size_t idx = (size_t)(k - '1');
            if (idx < recs.size()) watchReplay(recs[idx]);
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

    while (true) {
        std::cout << CLEAR;
        std::cout << "==============================\n";
        std::cout << "       Brave's Maze Tour\n";
        std::cout << "==============================\n\n";
        std::cout << "  [1] Play\n";
        std::cout << "  [2] Mode\n";
        std::cout << "  [3] Replay\n";
        std::cout << "  [4] Quit\n\n";
        std::cout << "Select 1-4...";

        char c = read_key();
        if (c == '1') {
            bool chosen = false;
            GameMode mode = chooseDifficulty(chosen);
            if (!chosen) continue;

            RunContext ctx;
            ctx.mode = mode;
            ctx.darkness = darkness;
            ctx.fullbright = fullbright;
            ctx.monsters = monsters;
            ctx.seed = makeSeed();

            Player player;
            long clearTime = 0;
            RunOutcome outcome = runGame(ctx, player, clearTime);

            if (outcome == RunOutcome::DIED) {
                std::cout << CLEAR;
                std::cout << "==================================================\n";
                std::cout << "                   Y O U   D I E D\n";
                std::cout << "==================================================\n\n";
                std::cout << "  A monster was too strong this time.\n";
                std::cout << "  Hero Lv " << player.getLevel()
                          << "    survived " << clearTime << "s\n\n";
                std::cout << "  Press any key to return to the menu\n";
                read_key();
            } else if (outcome == RunOutcome::COMPLETED) {
                std::cout << CLEAR;
                std::cout << "*** Floor 4 cleared -- you conquered the maze! ***\n\n";
                std::cout << "Press Enter to see your results...";
                while (true) {
                    char k = read_key();
                    if (k == '\r' || k == '\n' || k == ' ') break;
                }

                ScoreBreakdown sc = computeScore(mode, clearTime, darkness, fullbright);
                ReplayRecord rr;
                rr.seed = ctx.seed;
                rr.mode = (int)mode;
                rr.darkness = darkness ? 1 : 0;
                rr.fullbright = fullbright ? 1 : 0;
                rr.monsters = monsters ? 1 : 0;
                rr.clearTime = clearTime;
                rr.score = sc.total;
                rr.keys = ctx.recorded;
                saveReplay(rr, REPLAY_DIR);

                showResults(mode, darkness, fullbright, clearTime, player, false);
            }
        } else if (c == '2') {
            modeMenu(darkness, fullbright, monsters);
        } else if (c == '3') {
            replayMenu();
        } else if (c == '4' || c == 'q' || c == 'Q') {
            std::cout << CLEAR;
            std::cout << "Farewell, brave one!\n";
            return 0;
        }
    }
}
