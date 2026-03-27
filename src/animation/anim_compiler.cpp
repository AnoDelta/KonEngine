// anim_compiler.cpp
// Standalone tool — compile separately, run at build time.
//
// CLI usage:
//   anim_compiler <input.anim>              (outputs <input>.konani)
//   anim_compiler <input.anim> <output.konani>
//
// GUI usage:
//   anim_compiler                           (opens GUI window)
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

// -----------------------------------------------------------------------
// Shared data types
// -----------------------------------------------------------------------

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

struct Frame { float srcX, srcY, srcW, srcH, dur; };
struct Key   { float time, value; Ease curve; };
struct Track { std::string name; std::vector<Key> keys; };
struct Anim  { std::string name; bool loop = false; std::vector<Frame> frames; std::vector<Track> tracks; };

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------

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

// Strip extension and replace with .konani
static std::string toOutputPath(const std::string& input) {
    auto dot = input.rfind('.');
    std::string base = (dot != std::string::npos) ? input.substr(0, dot) : input;
    return base + ".konani";
}

// -----------------------------------------------------------------------
// Core compile logic — shared by CLI and GUI
// -----------------------------------------------------------------------

struct CompileResult {
    bool        success = false;
    std::string log;
    int         animCount = 0;
};

static CompileResult compile(const std::string& inputPath, const std::string& outputPath) {
    CompileResult result;

    std::ifstream in(inputPath);
    if (!in.is_open()) {
        result.log = "Error: Cannot open: " + inputPath;
        return result;
    }

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
            if (!cur) {
                result.log = "Error: Line " + std::to_string(lineNum) + ": frame outside anim";
                return result;
            }
            Frame f{};
            if (!(ss >> f.srcX >> f.srcY >> f.srcW >> f.srcH >> f.dur)) {
                result.log = "Error: Line " + std::to_string(lineNum) + ": bad frame";
                return result;
            }
            cur->frames.push_back(f);

        } else if (token == "track") {
            if (!cur) {
                result.log = "Error: Line " + std::to_string(lineNum) + ": track outside anim";
                return result;
            }
            std::string prop, curveStr = "easeinout";
            float time, value;
            if (!(ss >> prop >> time >> value)) {
                result.log = "Error: Line " + std::to_string(lineNum) + ": bad track";
                return result;
            }
            ss >> curveStr;
            auto it = curveMap.find(toLower(curveStr));
            Ease curve = (it != curveMap.end()) ? it->second : Ease::EaseInOut;
            findOrAddTrack(*cur, prop)->keys.push_back({ time, value, curve });

        } else if (token == "end") {
            cur = nullptr;
        } else {
            result.log = "Error: Line " + std::to_string(lineNum) + ": unknown token '" + token + "'";
            return result;
        }
    }

    std::ofstream out(outputPath, std::ios::binary);
    if (!out.is_open()) {
        result.log = "Error: Cannot write: " + outputPath;
        return result;
    }

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

    result.success   = true;
    result.animCount = static_cast<int>(anims.size());
    result.log       = "OK: Compiled " + std::to_string(anims.size()) +
                       " animation(s)\n    " + inputPath +
                       "\n -> " + outputPath;
    return result;
}

// -----------------------------------------------------------------------
// CLI entry point
// -----------------------------------------------------------------------

static int runCLI(int argc, char** argv) {
    std::string inputPath  = argv[1];
    std::string outputPath = (argc >= 3) ? argv[2] : toOutputPath(inputPath);

    CompileResult result = compile(inputPath, outputPath);
    if (result.success)
        std::cout << result.log << "\n";
    else
        std::cerr << result.log << "\n";

    return result.success ? 0 : 1;
}

// -----------------------------------------------------------------------
// GUI entry point (ImGui + GLFW + OpenGL)
// -----------------------------------------------------------------------

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

static int runGUI() {
    if (!glfwInit()) return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(600, 340, "KonEngine — Anim Compiler", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; // don't write imgui.ini next to the tool

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // State
    char inputBuf[512]  = "";
    char outputBuf[512] = "";
    std::string logText;
    bool        logSuccess = false;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize({600, 340});
        ImGui::Begin("##main", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize   |
            ImGuiWindowFlags_NoMove);

        ImGui::Text("KonEngine Anim Compiler");
        ImGui::Separator();
        ImGui::Spacing();

        // Input file
        ImGui::Text("Input (.anim)");
        ImGui::SetNextItemWidth(520);
        ImGui::InputText("##input", inputBuf, sizeof(inputBuf));
        ImGui::Spacing();

        // Output file
        ImGui::Text("Output (.konani)  —  leave blank to auto-name");
        ImGui::SetNextItemWidth(520);
        ImGui::InputText("##output", outputBuf, sizeof(outputBuf));
        ImGui::Spacing();

        // Compile button
        bool canCompile = inputBuf[0] != '\0';
        if (!canCompile) ImGui::BeginDisabled();
        if (ImGui::Button("Compile", {120, 32})) {
            std::string inp = inputBuf;
            std::string out = (outputBuf[0] != '\0') ? outputBuf : toOutputPath(inp);

            // Auto-fill output field so user can see what was written
            if (outputBuf[0] == '\0') {
                snprintf(outputBuf, sizeof(outputBuf), "%s", out.c_str());
            }

            CompileResult r = compile(inp, out);
            logText    = r.log;
            logSuccess = r.success;
        }
        if (!canCompile) ImGui::EndDisabled();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Log output
        if (!logText.empty()) {
            ImVec4 col = logSuccess
                ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f)   // green
                : ImVec4(1.0f, 0.4f, 0.4f, 1.0f);   // red
            ImGui::TextColored(col, "%s", logText.c_str());
        }

        // Hint at bottom
        ImGui::SetCursorPosY(300);
        ImGui::TextDisabled("Tip: you can also drag-and-drop a .anim file onto the executable.");

        ImGui::End();

        ImGui::Render();
        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

// -----------------------------------------------------------------------
// main
// -----------------------------------------------------------------------

int main(int argc, char** argv) {
    if (argc >= 2)
        return runCLI(argc, argv);
    else
        return runGUI();
}