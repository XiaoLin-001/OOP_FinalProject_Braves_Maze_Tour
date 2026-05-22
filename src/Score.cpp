#include "Score.h"

static const int BASE_EASY   = 1000;
static const int BASE_NORMAL = 2500;
static const int BASE_HARD   = 4000;

static const int PAR_EASY   = 120;
static const int PAR_NORMAL = 210;
static const int PAR_HARD   = 330;
static const int TIME_WEIGHT    = 8;
static const int TIME_BONUS_CAP = 1500;

static const double DARK_MULTIPLIER   = 1.5;
static const double BRIGHT_MULTIPLIER = 0.5;

const char* modeName(GameMode mode) {
    switch (mode) {
        case GameMode::EASY:   return "Easy";
        case GameMode::NORMAL: return "Normal";
        case GameMode::HARD:   return "Hard";
    }
    return "Unknown";
}

ScoreBreakdown computeScore(GameMode mode, long elapsedSeconds,
                            bool darkMode, bool fullBright) {
    ScoreBreakdown s;

    int par;
    switch (mode) {
        case GameMode::EASY:   s.base = BASE_EASY;   par = PAR_EASY;   break;
        case GameMode::NORMAL: s.base = BASE_NORMAL; par = PAR_NORMAL; break;
        case GameMode::HARD:   s.base = BASE_HARD;   par = PAR_HARD;   break;
        default:               s.base = BASE_EASY;   par = PAR_EASY;   break;
    }

    if (elapsedSeconds < 0) elapsedSeconds = 0;
    long under = (long)par - elapsedSeconds;
    int bonus = (under > 0) ? (int)(under * TIME_WEIGHT) : 0;
    if (bonus > TIME_BONUS_CAP) bonus = TIME_BONUS_CAP;
    s.timeBonus = bonus;

    double mult = 1.0;
    if (darkMode)   mult *= DARK_MULTIPLIER;
    if (fullBright) mult *= BRIGHT_MULTIPLIER;
    s.multiplier = mult;

    s.total = (int)((s.base + s.timeBonus) * s.multiplier + 0.5);
    return s;
}
