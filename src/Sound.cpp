#include "Sound.h"

#include <string>

#ifdef _WIN32
  #include <windows.h>
  #include <mmsystem.h>
#else
  #include <cstdlib>
#endif

static bool g_muted = false;

static void play(const char* name) {
    if (g_muted) return;
    std::string path = "assets/sfx/";
    path += name;
    path += ".wav";
#ifdef _WIN32
    PlaySoundA(path.c_str(), NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
#else
    std::string cmd = "{ aplay -q '" + path + "' || paplay '" + path + "'; }"
                      " >/dev/null 2>&1 &";
    if (system(cmd.c_str()) == -1) { /* no player available; stay silent */ }
#endif
}

namespace Sound {

void setMuted(bool m) {
    g_muted = m;
#ifdef _WIN32
    if (m) PlaySoundA(NULL, NULL, 0);
#endif
}

void move()       { play("move"); }
void select()     { play("select"); }
void back()       { play("back"); }
void toggle(bool on) { play(on ? "toggle_on" : "toggle_off"); }
void pickup()     { play("pickup"); }
void potion()     { play("potion"); }
void hit()        { play("hit"); }
void unbeatable() { play("unbeatable"); }
void levelUp()    { play("levelup"); }
void win()        { play("win"); }
void lose()       { play("lose"); }
void timeout()    { play("timeout"); }
void portal()     { play("portal"); }

}
