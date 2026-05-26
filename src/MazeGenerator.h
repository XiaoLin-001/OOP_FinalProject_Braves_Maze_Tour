#ifndef MAZE_GENERATOR_H
#define MAZE_GENERATOR_H

#include <vector>

namespace MazeGen {
    std::vector<std::vector<int>> generateDFS(int n);

    std::vector<std::vector<int>> generateKruskal(int n);
}

#endif
