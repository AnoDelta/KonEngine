// anim_compiler.cpp
// Standalone tool — compile separately, run at build time.
//
// CLI usage:
//   anim_compiler <input.anim>              (outputs <input>.konani)
//   anim_compiler <input.anim> <output.konani>
//
// GUI usage:
//   anim_compiler                           (opens Qt window)
//   anim_compiler <input.anim>              (CLI compile, no window)

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

static std::string toOutputPath(const std::string& input) {
    auto dot = input.rfind('.');
    std::string base = (dot != std::string::npos) ? input.substr(0, dot) : input;
    return base + ".konani";
}

// -----------------------------------------------------------------------
// Core compile logic — shared by CLI and GUI
// -----------------------------------------------------------------------

struct CompileResult {
    bool        success   = false;
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
            if (!cur) { result.log = "Error: Line " + std::to_string(lineNum) + ": frame outside anim"; return result; }
            Frame f{};
            if (!(ss >> f.srcX >> f.srcY >> f.srcW >> f.srcH >> f.dur)) {
                result.log = "Error: Line " + std::to_string(lineNum) + ": bad frame"; return result;
            }
            cur->frames.push_back(f);

        } else if (token == "track") {
            if (!cur) { result.log = "Error: Line " + std::to_string(lineNum) + ": track outside anim"; return result; }
            std::string prop, curveStr = "easeinout";
            float time, value;
            if (!(ss >> prop >> time >> value)) {
                result.log = "Error: Line " + std::to_string(lineNum) + ": bad track"; return result;
            }
            ss >> curveStr;
            auto it = curveMap.find(toLower(curveStr));
            Ease curve = (it != curveMap.end()) ? it->second : Ease::EaseInOut;
            findOrAddTrack(*cur, prop)->keys.push_back({ time, value, curve });

        } else if (token == "display") {
            // Accepted and ignored — display metadata is KonAnimator's concern
            // (the anim_compiler binary format doesn't include it)

        } else if (token == "spritesheet") {
            // Accepted and ignored

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
// Qt GUI
// -----------------------------------------------------------------------

#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QStyleFactory>
#include <QPalette>
#include <QSizePolicy>

class CompilerWindow : public QMainWindow {
    Q_OBJECT
public:
    CompilerWindow() {
        setWindowTitle("KonEngine — Anim Compiler");
        setMinimumSize(560, 300);
        resize(600, 320);
        setAcceptDrops(true);

        auto* central = new QWidget(this);
        setCentralWidget(central);
        auto* vl = new QVBoxLayout(central);
        vl->setContentsMargins(12, 12, 12, 12);
        vl->setSpacing(8);

        // Input row
        {
            auto* row = new QHBoxLayout;
            row->addWidget(new QLabel("Input (.anim):"));
            m_inputEdit = new QLineEdit;
            m_inputEdit->setPlaceholderText("path/to/animation.anim");
            row->addWidget(m_inputEdit, 1);
            auto* browse = new QPushButton("Browse…");
            browse->setFixedWidth(80);
            connect(browse, &QPushButton::clicked, this, &CompilerWindow::browseInput);
            row->addWidget(browse);
            vl->addLayout(row);
        }

        // Output row
        {
            auto* row = new QHBoxLayout;
            row->addWidget(new QLabel("Output (.konani):"));
            m_outputEdit = new QLineEdit;
            m_outputEdit->setPlaceholderText("leave blank to auto-name");
            row->addWidget(m_outputEdit, 1);
            auto* browse = new QPushButton("Browse…");
            browse->setFixedWidth(80);
            connect(browse, &QPushButton::clicked, this, &CompilerWindow::browseOutput);
            row->addWidget(browse);
            vl->addLayout(row);
        }

        // Compile button
        {
            auto* row = new QHBoxLayout;
            m_compileBtn = new QPushButton("Compile");
            m_compileBtn->setFixedHeight(32);
            m_compileBtn->setEnabled(false);
            connect(m_compileBtn, &QPushButton::clicked, this, &CompilerWindow::doCompile);
            row->addWidget(m_compileBtn);
            row->addStretch();
            auto* hint = new QLabel("Tip: drag and drop a .anim file onto this window");
            hint->setStyleSheet("color: gray; font-size: 9pt;");
            row->addWidget(hint);
            vl->addLayout(row);
        }

        // Log output
        m_log = new QTextEdit;
        m_log->setReadOnly(true);
        m_log->setFont(QFont("monospace", 9));
        m_log->setMinimumHeight(80);
        vl->addWidget(m_log, 1);

        connect(m_inputEdit, &QLineEdit::textChanged, this, [this](const QString& t) {
            m_compileBtn->setEnabled(!t.trimmed().isEmpty());
        });
    }

    void setInputPath(const QString& path) {
        m_inputEdit->setText(path);
    }

protected:
    void dragEnterEvent(QDragEnterEvent* e) override {
        if (e->mimeData()->hasUrls()) e->acceptProposedAction();
    }
    void dropEvent(QDropEvent* e) override {
        const auto urls = e->mimeData()->urls();
        if (!urls.isEmpty()) {
            QString path = urls.first().toLocalFile();
            if (path.endsWith(".anim", Qt::CaseInsensitive))
                m_inputEdit->setText(path);
        }
    }

private slots:
    void browseInput() {
        QString path = QFileDialog::getOpenFileName(this,
            "Open .anim file", QString(), "Animation files (*.anim);;All files (*)");
        if (!path.isEmpty()) m_inputEdit->setText(path);
    }

    void browseOutput() {
        QString path = QFileDialog::getSaveFileName(this,
            "Save .konani file", QString(), "Compiled animation (*.konani);;All files (*)");
        if (!path.isEmpty()) m_outputEdit->setText(path);
    }

    void doCompile() {
        QString inp = m_inputEdit->text().trimmed();
        QString out = m_outputEdit->text().trimmed();
        if (out.isEmpty()) {
            out = QString::fromStdString(toOutputPath(inp.toStdString()));
            m_outputEdit->setText(out);
        }

        CompileResult r = compile(inp.toStdString(), out.toStdString());

        m_log->clear();
        if (r.success) {
            m_log->setTextColor(QColor(80, 220, 80));
        } else {
            m_log->setTextColor(QColor(220, 80, 80));
        }
        m_log->setText(QString::fromStdString(r.log));
    }

private:
    QLineEdit*  m_inputEdit  = nullptr;
    QLineEdit*  m_outputEdit = nullptr;
    QPushButton* m_compileBtn = nullptr;
    QTextEdit*  m_log        = nullptr;
};

static int runGUI(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setApplicationName("KonAnimator — Anim Compiler");
    app.setOrganizationName("KonEngine");

    // Match KonAnimator's dark Fusion theme
    app.setStyle(QStyleFactory::create("Fusion"));
    QPalette pal;
    pal.setColor(QPalette::Window,          QColor(45,45,45));
    pal.setColor(QPalette::WindowText,      QColor(220,220,220));
    pal.setColor(QPalette::Base,            QColor(30,30,30));
    pal.setColor(QPalette::AlternateBase,   QColor(40,40,40));
    pal.setColor(QPalette::ToolTipBase,     QColor(55,55,55));
    pal.setColor(QPalette::ToolTipText,     QColor(220,220,220));
    pal.setColor(QPalette::Text,            QColor(220,220,220));
    pal.setColor(QPalette::Button,          QColor(55,55,55));
    pal.setColor(QPalette::ButtonText,      QColor(220,220,220));
    pal.setColor(QPalette::BrightText,      Qt::red);
    pal.setColor(QPalette::Highlight,       QColor(0,150,220));
    pal.setColor(QPalette::HighlightedText, Qt::white);
    app.setPalette(pal);

    CompilerWindow win;
    win.show();

    return app.exec();
}

// -----------------------------------------------------------------------
// main
// -----------------------------------------------------------------------

int main(int argc, char** argv) {
    if (argc >= 2)
        return runCLI(argc, argv);
    else
        return runGUI(argc, argv);
}

#include "anim_compiler.moc"
