#ifndef REPLAY_H
#define REPLAY_H

#include <vector>
#include <string>

struct ReplayRecord {
    unsigned seed;
    int mode;
    int darkness;
    int fullbright;
    int monsters;
    long clearTime;
    int score;
    std::vector<int> keys;
    std::string name;
};

bool saveReplay(const ReplayRecord& r, const std::string& dir);
std::vector<ReplayRecord> loadReplays(const std::string& dir);

#endif
