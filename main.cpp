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

    std::cout << "==============================\n";
    std::cout << "      Brave's Maze Tour\n";
    std::cout << "==============================\n";
    std::cout << "Press Enter to begin...";
    std::cin.get();

    for (int level = 1; level <= 4; ++level) {
        std::string fname = "maze_" + std::to_string(level) + ".txt";
        Maze maze(fname, level);
        if (maze.get_nRow() == 0) {
            std::cerr << "Failed to load " << fname << "\n";
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
        std::cout << "Press Enter for the next floor...";
        std::cin.get();
        std::cin.get();
    }

    std::cout << "\033[2J\033[1;1H";
    std::cout << "Congratulations! You conquered all 4 floors of the maze!\n";
    return 0;
}
