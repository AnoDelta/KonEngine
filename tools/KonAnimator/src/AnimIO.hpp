#pragma once
#include "AnimData.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>

namespace AnimIO {

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------
static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}
static Ease parseEase(const std::string& s) {
    std::string lo = toLower(s);
    for (int i = 0; i < kEaseCount; i++)
        if (lo == kEaseLower[i]) return static_cast<Ease>(i);
    return Ease::EaseInOut;
}
static void wStr(std::ofstream& o, const std::string& s) {
    uint32_t len = (uint32_t)s.size();
    o.write((char*)&len, 4);
    o.write(s.data(), len);
}
static void wF(std::ofstream& o, float v)    { o.write((char*)&v, 4); }
static void wU32(std::ofstream& o, uint32_t v){ o.write((char*)&v, 4); }

// -----------------------------------------------------------------------
// Load .anim text file
// -----------------------------------------------------------------------
inline bool load(const std::string& path, AnimProject& proj, std::string& err) {
    std::ifstream f(path);
    if (!f.is_open()) { err = "Cannot open: " + path; return false; }

    proj.clips.clear();
    proj.filePath = path;
    proj.dirty    = false;

    AnimClip* cur = nullptr;
    std::string line;
    int lineNum = 0;

    while (std::getline(f, line)) {
        ++lineNum;
        if (auto c = line.find('#'); c != std::string::npos) line = line.substr(0, c);
        auto s = line.find_first_not_of(" \t");
        if (s == std::string::npos) continue;
        line = line.substr(s);
        if (line.empty()) continue;

        std::istringstream ss(line);
        std::string tok; ss >> tok;

        if (tok == "spritesheet") {
            ss >> proj.spritesheetPath;

        } else if (tok == "anim") {
            proj.clips.emplace_back();
            cur = &proj.clips.back();
            ss >> cur->name;
            std::string flag;
            while (ss >> flag) if (flag == "loop") cur->loop = true;

        } else if (tok == "display") {
            if (!cur) { err = "Line " + std::to_string(lineNum) + ": display outside anim"; return false; }
            ss >> cur->displayW >> cur->displayH >> cur->displayScale;

        } else if (tok == "frame") {
            if (!cur) { err = "Line " + std::to_string(lineNum) + ": frame outside anim"; return false; }
            AnimFrame fr;
            if (!(ss >> fr.srcX >> fr.srcY >> fr.srcW >> fr.srcH >> fr.duration)) {
                err = "Line " + std::to_string(lineNum) + ": bad frame"; return false;
            }
            cur->frames.push_back(fr);

        } else if (tok == "track") {
            if (!cur) { err = "Line " + std::to_string(lineNum) + ": track outside anim"; return false; }
            std::string prop, curveStr = "easeinout";
            float time, value;
            if (!(ss >> prop >> time >> value)) {
                err = "Line " + std::to_string(lineNum) + ": bad track"; return false;
            }
            ss >> curveStr;
            cur->getOrAddTrack(prop).keys.push_back({time, value, parseEase(curveStr)});

        } else if (tok == "end") {
            cur = nullptr;
        } else {
            err = "Line " + std::to_string(lineNum) + ": unknown token '" + tok + "'";
            return false;
        }
    }
    return true;
}

// -----------------------------------------------------------------------
// Save .anim text file
// -----------------------------------------------------------------------
inline bool save(const AnimProject& proj, std::string& err) {
    if (proj.filePath.empty()) { err = "No file path"; return false; }
    std::ofstream f(proj.filePath);
    if (!f.is_open()) { err = "Cannot write: " + proj.filePath; return false; }

    if (!proj.spritesheetPath.empty())
        f << "spritesheet " << proj.spritesheetPath << "\n\n";

    for (auto& c : proj.clips) {
        f << "anim " << c.name << (c.loop ? " loop" : "") << "\n";
        f << "\tdisplay " << c.displayW << " " << c.displayH << " " << c.displayScale << "\n";
        for (auto& fr : c.frames)
            f << "\tframe " << fr.srcX << " " << fr.srcY << " "
              << fr.srcW   << " " << fr.srcH   << " " << fr.duration << "\n";
        for (auto& tr : c.tracks)
            for (auto& kf : tr.keys)
                f << "\ttrack " << tr.name << " " << kf.time << " "
                  << kf.value << " " << kEaseLower[(int)kf.curve] << "\n";
        f << "end\n\n";
    }
    return true;
}

// -----------------------------------------------------------------------
// Compile to .konani binary
// -----------------------------------------------------------------------
inline bool compile(const AnimProject& proj, const std::string& outPath, std::string& err) {
    std::ofstream f(outPath, std::ios::binary);
    if (!f.is_open()) { err = "Cannot write: " + outPath; return false; }

    wU32(f, (uint32_t)proj.clips.size());
    for (auto& c : proj.clips) {
        wStr(f, c.name);
        uint8_t loop = c.loop ? 1 : 0;
        f.write((char*)&loop, 1);

        // display metadata (new — engine AnimationPlayer must read these)
        wF(f, c.displayW);
        wF(f, c.displayH);
        wF(f, c.displayScale);

        wU32(f, (uint32_t)c.frames.size());
        for (auto& fr : c.frames) {
            wF(f, fr.srcX); wF(f, fr.srcY);
            wF(f, fr.srcW); wF(f, fr.srcH);
            wF(f, fr.duration);
        }

        wU32(f, (uint32_t)c.tracks.size());
        for (auto& tr : c.tracks) {
            wStr(f, tr.name);
            wU32(f, (uint32_t)tr.keys.size());
            for (auto& kf : tr.keys) {
                wF(f, kf.time);
                wF(f, kf.value);
                wU32(f, (uint32_t)kf.curve);
            }
        }
    }
    return true;
}

inline std::string konaniPath(const std::string& animPath) {
    auto dot = animPath.rfind('.');
    return (dot != std::string::npos ? animPath.substr(0, dot) : animPath) + ".konani";
}

} // namespace AnimIO
