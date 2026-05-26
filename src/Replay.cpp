#include "Replay.h"

#include <fstream>
#include <sstream>
#include <ctime>
#include <algorithm>

#ifdef _WIN32
  #include <direct.h>
  #include <windows.h>
#else
  #include <sys/stat.h>
  #include <dirent.h>
#endif

static void ensureDir(const std::string& dir) {
#ifdef _WIN32
    _mkdir(dir.c_str());
#else
    mkdir(dir.c_str(), 0755);
#endif
}

static std::string timestampName(unsigned seed) {
    time_t t = time(nullptr);
    struct tm* lt = localtime(&t);
    char buf[64];
    strftime(buf, sizeof(buf), "replay_%Y%m%d_%H%M%S", lt);
    std::ostringstream os;
    os << buf << "_" << (seed % 100000u) << ".txt";
    return os.str();
}

bool saveReplay(const ReplayRecord& r, const std::string& dir) {
    ensureDir(dir);
    std::string path = dir + "/" + timestampName(r.seed);
    std::ofstream f(path.c_str());
    if (!f) return false;
    f << r.seed << " " << r.mode << " " << r.darkness << " " << r.fullbright
      << " " << r.monsters << " " << r.timeattack << " " << r.clearTime << " " << r.score
      << " " << r.keys.size();
    for (size_t i = 0; i < r.keys.size(); ++i)
        f << " " << r.keys[i];
    f << "\n";
    return true;
}

static std::vector<std::string> listTxt(const std::string& dir) {
    std::vector<std::string> names;
#ifdef _WIN32
    std::string pattern = dir + "\\*.txt";
    WIN32_FIND_DATA fd;
    HANDLE h = FindFirstFile(pattern.c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do { names.push_back(fd.cFileName); } while (FindNextFile(h, &fd));
        FindClose(h);
    }
#else
    DIR* d = opendir(dir.c_str());
    if (d) {
        struct dirent* e;
        while ((e = readdir(d)) != nullptr) {
            std::string n = e->d_name;
            if (n.size() > 4 && n.substr(n.size() - 4) == ".txt")
                names.push_back(n);
        }
        closedir(d);
    }
#endif
    std::sort(names.begin(), names.end());
    return names;
}

static bool parseRecord(const std::string& path, ReplayRecord& r) {
    std::ifstream f(path.c_str());
    if (!f) return false;
    size_t n = 0;
    if (!(f >> r.seed >> r.mode >> r.darkness >> r.fullbright >> r.monsters
            >> r.timeattack >> r.clearTime >> r.score >> n))
        return false;
    r.keys.clear();
    r.keys.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        int k;
        if (f >> k) r.keys.push_back(k);
    }
    return true;
}

std::vector<ReplayRecord> loadReplays(const std::string& dir) {
    std::vector<ReplayRecord> out;
    std::vector<std::string> files = listTxt(dir);
    for (size_t i = 0; i < files.size(); ++i) {
        ReplayRecord r;
        if (parseRecord(dir + "/" + files[i], r)) {
            r.name = files[i];
            out.push_back(r);
        }
    }
    return out;
}
