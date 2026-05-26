#ifndef SCORE_H
#define SCORE_H

#include "Maze.h"

struct ScoreBreakdown {
    int    base;
    int    timeBonus;
    int    monsterBonus;
    int    expBonus;
    double multiplier;
    int    total;
};

ScoreBreakdown computeScore(GameMode mode, long elapsedSeconds,
                            bool darkMode, bool fullBright, bool timeAttack,
                            int monstersDefeated, int totalExp);

const char* modeName(GameMode mode);

#endif
