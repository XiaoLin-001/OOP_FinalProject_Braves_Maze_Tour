#ifndef MAZE_GENERATOR_H
#define MAZE_GENERATOR_H

#include <vector>

// ----------------------------------------------------------------------------
// MazeGenerator : produces a maze as a grid of integers (the same 0/1/2 format
// used by maze_*.txt) so the rest of the game (Maze loading, item placement,
// rendering) does not need to change.
//
//   0 = Empty (corridor)   1 = Wall   2 = Goal
//
// The grid uses the classic "odd-coordinate" layout: rooms sit on odd indices
// and walls sit between them, so n must be odd. Both algorithms produce a
// "perfect maze" (exactly one path between any two cells).
// ----------------------------------------------------------------------------
namespace MazeGen {
    // Recursive Backtracker (DFS): long winding corridors, few junctions.
    std::vector<std::vector<int>> generateDFS(int n);

    // Randomized Kruskal's + Union-Find: many short dead ends, very uniform.
    std::vector<std::vector<int>> generateKruskal(int n);
}

#endif // MAZE_GENERATOR_H
