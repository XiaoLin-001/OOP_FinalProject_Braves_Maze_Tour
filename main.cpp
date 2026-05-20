#include "Maze.h"
#include "Player.h"
#include "Block.h"

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <string>

// ----------------------------------------------------------------------------
// Brave's Maze Tour
// Explore four floors. On floor N collect all N keys, then reach the Goal.
// ----------------------------------------------------------------------------
int main() {
    srand((unsigned)time(nullptr));   // different item layout every run

    Player player;

    // --- Mode selection ---
    GameMode mode = GameMode::EASY;
    while (true) {
        std::cout << "\033[2J\033[1;1H";
        std::cout << "==============================\n";
        std::cout << "      Brave's Maze Tour\n";
        std::cout << "==============================\n\n";
        std::cout << "Choose a mode:\n\n";
        std::cout << "  [1] Easy   - tutorial, fixed maze\n";
        std::cout << "  [2] Normal - DFS generated maze\n";
        std::cout << "  [3] Hard   - Kruskal generated maze\n\n";
        std::cout << "Press 1 / 2 / 3 to start...";

        char c = player.get_direction();
        if (c == '1') { mode = GameMode::EASY;   break; }
        if (c == '2') { mode = GameMode::NORMAL; break; }
        if (c == '3') { mode = GameMode::HARD;   break; }
        if (c == 'e' || c == 'E') return 0;
    }

    for (int level = 1; level <= 4; ++level) {
        Maze maze(level, mode);
        if (maze.get_nRow() == 0) {
            std::cerr << "Failed to build maze for floor " << level << "\n";
            return 1;
        }

        maze.placeItems();
        player.resetForLevel();
        maze.reveal(player.getRow(), player.getCol());   // start with the 8 cells around us

        bool quit = false;
        while (!maze.isCleared()) {
            maze.printMaze(player);
            char dir = player.get_direction();

            if (dir == 'e' || dir == 'E') { quit = true; break; }

            maze.setMessage("");                 // clear last status line
            bool moved = player.move(dir, maze);

            // On floors 3 & 4 the Goal wanders after the player moves.
            if (moved && maze.usesMovableGoal() && !maze.isCleared())
                maze.moveGoalRandom();
        }

        if (quit) {
            std::cout << "\nYou left the maze. Farewell, brave one!\n";
            return 0;
        }

        std::cout << "\033[2J\033[1;1H";
        std::cout << "*** Floor " << level << " cleared! ***\n";
        std::cout << "Press any key for the next floor...";
        player.get_direction();
    }

    std::cout << "\033[2J\033[1;1H";
    std::cout << "Congratulations! You conquered all 4 floors of the maze!\n";
    return 0;
}
