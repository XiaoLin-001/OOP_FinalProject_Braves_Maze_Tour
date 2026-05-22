#ifndef SCORE_H
#define SCORE_H

#include "Maze.h"

struct ScoreBreakdown {
    int    base;
    int    timeBonus;
    double multiplier;
    int    total;
};

ScoreBreakdown computeScore(GameMode mode, long elapsedSeconds,
                            bool darkMode, bool fullBright);

const char* modeName(GameMode mode);

#endif
