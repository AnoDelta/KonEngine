// anim_compiler.cpp
// Standalone tool — compile separately, run at build time.
// Usage: anim_compiler <input.anim> <output.animb>
//
// -----------------------------------------------------------------------
// .anim text format
// -----------------------------------------------------------------------
//
// # Sprite sheet animation
// anim idle loop
//   frame 0 0 32 32 0.15
//   frame 32 0 32 32 0.15
// end
//
// # Keyframe animation (property tracks)
// # track <property> <time> <value> [curve]
// # Properties: x  y  scaleX  scaleY  rotation  alpha
// # Curves: linear easein easeout easeinout
// #         easeincubic easeoutcubic easeinoutcubic
// #         easeinelastic easeoutelastic easeinoutelastic
// #         easeinbounce easeoutbounce easeinoutbounce
// #         easeinback easeoutback easeinoutback
//
// anim pop_in
//   track scaleX 0.0 0.0 easeinoutback
//   track scaleX 0.4 1.0 easeinoutback
//   track scaleY 0.0 0.0 easeinoutback
//   track scaleY 0.4 1.0 easeinoutback
//   track alpha  0.0 0.0 easeout
//   track alpha  0.3 1.0 easeout
// end

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cstdint>

enum class Ease : uint32_t {
    Linear,
    EaseIn, EaseOut, EaseInOut,
    EaseInCubic, EaseOutCubic, EaseInOutCubic,
    EaseInElastic, EaseOutElastic, EaseInOutElastic,
    EaseInBounce, EaseOutBounce, EaseInOutBounce,
    EaseInBack, EaseOutBack, EaseInOutBack,
};

static const std::unordered_map<std::string, Ease> curveMap = {
    {"linear",           Ease::Linear},
    {"easein",           Ease::EaseIn},
    {"easeout",          Ease::EaseOut},
    {"easeinout",        Ease::EaseInOut},
    {"easeincubic",      Ease::EaseInCubic},
    {"easeoutcubic",     Ease::EaseOutCubic},
    {"easeinoutcubic",   Ease::EaseInOutCubic},
    {"easeinelastic",    Ease::EaseInElastic},
    {"easeoutelastic",   Ease::EaseOutElastic},
    {"easeinoutelastic", Ease::EaseInOutElastic},
    {"easeinbounce",     Ease::EaseInBounce},
    {"easeoutbounce",    Ease::EaseOutBounce},
    {"easeinoutbounce",  Ease::EaseInOutBounce},
    {"easeinback",       Ease::EaseInBack},
    {"easeoutback",      Ease::EaseOutBack},
    {"easeinoutback",    Ease::EaseInOutBack},
};

struct Frame  { float srcX, srcY, srcW, srcH, dur; };
struct Key    { float time, value; Ease curve; };
struct Track  { std::string name; std::vector<Key> keys; };
struct Anim   { std::string name; bool loop = false; std::vector<Frame> frames; std::vector<Track> tracks; };

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

static Track* findOrAddTrack(Anim& anim, const std::string& name) {
    for (auto& t : anim.tracks) if (t.name == name) return &t;
    anim.tracks.push_back({ name, {} });
    return &anim.tracks.back();
}

static void writeStr(std::ofstream& o, const std::string& s) {
    uint32_t len = static_cast<uint32_t>(s.size());
    o.write(reinterpret_cast<const char*>(&len), sizeof(len));
    o.write(s.data(), len);
}
static void writeF  (std::ofstream& o, float v)    { o.write(reinterpret_cast<const char*>(&v), sizeof(v)); }
static void writeU32(std::ofstream& o, uint32_t v) { o.write(reinterpret_cast<const char*>(&v), sizeof(v)); }

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: anim_compiler <input.anim> <output.animb>\n";
        return 1;
    }

    std::ifstream in(argv[1]);
    if (!in.is_open()) { std::cerr << "Cannot open: " << argv[1] << "\n"; return 1; }

    std::vector<Anim> anims;
    Anim* cur = nullptr;
    std::string line;
    int lineNum = 0;

    while (std::getline(in, line)) {
        lineNum++;
        auto c = line.find('#');
        if (c != std::string::npos) line = line.substr(0, c);
        size_t s = line.find_first_not_of(" \t");
        if (s == std::string::npos) continue;
        line = line.substr(s);
        if (line.empty()) continue;

        std::istringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "anim") {
            anims.emplace_back();
            cur = &anims.back();
            ss >> cur->name;
            std::string flag;
            while (ss >> flag) if (flag == "loop") cur->loop = true;

        } else if (token == "frame") {
            if (!cur) { std::cerr << "Line " << lineNum << ": frame outside anim\n"; return 1; }
            Frame f{};
            if (!(ss >> f.srcX >> f.srcY >> f.srcW >> f.srcH >> f.dur)) {
                std::cerr << "Line " << lineNum << ": bad frame\n"; return 1;
            }
            cur->frames.push_back(f);

        } else if (token == "track") {
            if (!cur) { std::cerr << "Line " << lineNum << ": track outside anim\n"; return 1; }
            std::string prop, curveStr = "easeinout";
            float time, value;
            if (!(ss >> prop >> time >> value)) {
                std::cerr << "Line " << lineNum << ": bad track\n"; return 1;
            }
            ss >> curveStr;
            auto it = curveMap.find(toLower(curveStr));
            Ease curve = (it != curveMap.end()) ? it->second : Ease::EaseInOut;
            findOrAddTrack(*cur, prop)->keys.push_back({ time, value, curve });

        } else if (token == "end") {
            cur = nullptr;
        } else {
            std::cerr << "Line " << lineNum << ": unknown token '" << token << "'\n";
            return 1;
        }
    }

    std::ofstream out(argv[2], std::ios::binary);
    if (!out.is_open()) { std::cerr << "Cannot write: " << argv[2] << "\n"; return 1; }

    writeU32(out, static_cast<uint32_t>(anims.size()));

    for (auto& anim : anims) {
        writeStr(out, anim.name);
        uint8_t loop = anim.loop ? 1 : 0;
        out.write(reinterpret_cast<const char*>(&loop), 1);

        writeU32(out, static_cast<uint32_t>(anim.frames.size()));
        for (auto& f : anim.frames) {
            writeF(out, f.srcX); writeF(out, f.srcY);
            writeF(out, f.srcW); writeF(out, f.srcH);
            writeF(out, f.dur);
        }

        writeU32(out, static_cast<uint32_t>(anim.tracks.size()));
        for (auto& t : anim.tracks) {
            writeStr(out, t.name);
            writeU32(out, static_cast<uint32_t>(t.keys.size()));
            for (auto& k : t.keys) {
                writeF(out, k.time);
                writeF(out, k.value);
                writeU32(out, static_cast<uint32_t>(k.curve));
            }
        }
    }

    std::cout << "Compiled " << anims.size() << " animation(s) -> " << argv[2] << "\n";
    return 0;
}
