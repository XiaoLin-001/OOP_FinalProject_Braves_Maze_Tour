import wave, struct, math, os, random

random.seed(1234)

SR = 44100
OUT = os.path.join(os.path.dirname(__file__), "..", "assets", "sfx")
os.makedirs(OUT, exist_ok=True)

def env(i, n, attack=0.005, release=0.06, tau=0.18):
    t = i / SR
    total = n / SR
    a = min(1.0, t / attack) if attack > 0 else 1.0
    r = min(1.0, (total - t) / release) if release > 0 else 1.0
    d = math.exp(-t / tau) if tau else 1.0
    return a * max(0.0, r) * d

def wave_val(kind, phase):
    if kind == "sine":
        return math.sin(phase)
    if kind == "tri":
        x = (phase / (2 * math.pi)) % 1.0
        return 4 * abs(x - 0.5) - 1
    if kind == "square":
        return 1.0 if math.sin(phase) >= 0 else -1.0
    if kind == "saw":
        x = (phase / (2 * math.pi)) % 1.0
        return 2 * x - 1
    return math.sin(phase)

def note(freq, dur, kind="sine", vol=0.3, sweep_to=None,
         harmonics=(), noise=0.0, attack=0.005, release=0.06, tau=0.18,
         vibrato=0.0, vib_rate=6.0):
    n = int(SR * dur)
    out = [0.0] * n
    phase = 0.0
    for i in range(n):
        frac = i / n
        f = freq if sweep_to is None else freq + (sweep_to - freq) * frac
        if vibrato:
            f *= 1.0 + vibrato * math.sin(2 * math.pi * vib_rate * i / SR)
        phase += 2 * math.pi * f / SR
        s = wave_val(kind, phase)
        for (mult, amp) in harmonics:
            s += amp * wave_val(kind, phase * mult)
        if noise:
            s += noise * (random.random() * 2 - 1)
        out[i] = s * vol * env(i, n, attack, release, tau)
    return out

def mix(*sigs):
    n = max(len(s) for s in sigs)
    out = [0.0] * n
    for s in sigs:
        for i in range(len(s)):
            out[i] += s[i]
    return out

def seq(*sigs):
    out = []
    for s in sigs:
        out.extend(s)
    return out

def pad(sig, ms):
    return sig + [0.0] * int(SR * ms / 1000.0)

def save(name, sig):
    peak = max(1e-6, max(abs(x) for x in sig))
    norm = min(1.0, 0.85 / peak)
    path = os.path.join(OUT, name + ".wav")
    w = wave.open(path, "w")
    w.setnchannels(1)
    w.setsampwidth(2)
    w.setframerate(SR)
    frames = bytearray()
    for x in sig:
        v = int(max(-1.0, min(1.0, x * norm)) * 32767)
        frames += struct.pack("<h", v)
    w.writeframes(bytes(frames))
    w.close()
    print("wrote", path, "(%.2fs)" % (len(sig) / SR))

HARM = ((2, 0.25), (3, 0.12))

save("move", note(720, 0.05, "sine", 0.28, harmonics=((2, 0.15),), tau=0.05, release=0.02))

save("select", seq(
    note(660, 0.07, "tri", 0.3, harmonics=HARM, tau=0.12, release=0.03),
    note(990, 0.12, "tri", 0.32, harmonics=HARM, tau=0.14)))

save("back", seq(
    note(620, 0.07, "tri", 0.3, harmonics=HARM, tau=0.1, release=0.03),
    note(440, 0.11, "tri", 0.3, harmonics=HARM, tau=0.13)))

save("toggle_on", seq(
    note(784, 0.06, "sine", 0.3, harmonics=HARM, tau=0.1, release=0.03),
    note(1175, 0.13, "sine", 0.32, harmonics=HARM, tau=0.16)))

save("toggle_off", seq(
    note(660, 0.06, "sine", 0.3, harmonics=HARM, tau=0.1, release=0.03),
    note(440, 0.13, "sine", 0.3, harmonics=HARM, tau=0.16)))

save("pickup", seq(
    note(659, 0.06, "sine", 0.26, tau=0.1, release=0.03, attack=0.008),
    note(988, 0.2, "sine", 0.3, harmonics=((2, 0.06),), tau=0.22, release=0.05, attack=0.008)))

save("potion", seq(
    note(1046, 0.06, "sine", 0.26, harmonics=((2, 0.2),), tau=0.1, release=0.02),
    note(1318, 0.06, "sine", 0.26, harmonics=((2, 0.2),), tau=0.1, release=0.02),
    note(1568, 0.18, "sine", 0.28, harmonics=((2, 0.2),), tau=0.2)))

save("hit", mix(
    note(150, 0.16, "sine", 0.5, sweep_to=80, tau=0.08, release=0.05),
    note(220, 0.1, "tri", 0.2, noise=0.35, tau=0.05, release=0.04)))

save("unbeatable", seq(
    note(140, 0.18, "saw", 0.34, harmonics=((2, 0.2),), tau=0.4, vibrato=0.03, vib_rate=7),
    note(105, 0.28, "saw", 0.34, harmonics=((2, 0.2),), tau=0.5, vibrato=0.03, vib_rate=7)))

save("levelup", seq(
    note(523, 0.09, "tri", 0.3, harmonics=HARM, tau=0.16, release=0.03),
    note(659, 0.09, "tri", 0.3, harmonics=HARM, tau=0.16, release=0.03),
    note(784, 0.09, "tri", 0.3, harmonics=HARM, tau=0.16, release=0.03),
    note(1046, 0.28, "tri", 0.34, harmonics=HARM, tau=0.3)))

save("win", seq(
    note(523, 0.13, "tri", 0.3, harmonics=HARM, tau=0.25, release=0.04),
    note(659, 0.13, "tri", 0.3, harmonics=HARM, tau=0.25, release=0.04),
    note(784, 0.13, "tri", 0.3, harmonics=HARM, tau=0.25, release=0.04),
    mix(
        note(1046, 0.5, "tri", 0.3, harmonics=HARM, tau=0.5),
        note(1318, 0.5, "tri", 0.22, harmonics=HARM, tau=0.5),
        note(1568, 0.5, "tri", 0.18, harmonics=HARM, tau=0.5))))

save("lose", seq(
    note(440, 0.18, "tri", 0.32, harmonics=((2, 0.15),), tau=0.3, release=0.05),
    note(330, 0.2, "tri", 0.32, harmonics=((2, 0.15),), tau=0.32, release=0.05),
    note(220, 0.45, "tri", 0.34, harmonics=((2, 0.15),), tau=0.5)))

save("timeout", seq(
    note(700, 0.1, "square", 0.22, tau=0.2, release=0.03),
    pad(note(520, 0.1, "square", 0.22, tau=0.2, release=0.03), 40),
    note(700, 0.1, "square", 0.22, tau=0.2, release=0.03),
    note(520, 0.22, "square", 0.24, tau=0.25)))

save("portal", note(400, 0.26, "sine", 0.32, sweep_to=1600,
                    harmonics=((2, 0.2),), tau=0.4, release=0.05, attack=0.01))

print("done")
