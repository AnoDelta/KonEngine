#include "ProjectManager.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>

ProjectManager::ProjectManager(QObject* parent) : QObject(parent) {}

bool ProjectManager::create(const QString& dir) {
    QDir d(dir);
    QString name = d.dirName();
    if (name.isEmpty()) name = "MyGame";

    // Create folder structure
    d.mkpath("src");
    d.mkpath("assets");
    d.mkpath("build");

    // Write starter main.ks
    QString mainKs = dir + "/src/main.ks";
    if (!QFile::exists(mainKs)) {
        QFile f(mainKs);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            f.write(R"(#include <engine>

func main() {
    InitWindow(800, 600, ")" + name.toUtf8() + R"(")
    SetTargetFPS(60)

    while !WindowShouldClose() {
        ClearBackground(0.1, 0.1, 0.15)

        Present()
        PollEvents()
    }
}
)");
        }
    }

    // Write .konproj
    m_json = QJsonObject{
        {"name",    name},
        {"version", "0.1.0"},
        {"entry",   "src/main.ks"},
        {"created", QDateTime::currentDateTime().toString(Qt::ISODate)}
    };

    m_path      = dir + "/" + name + ".konproj";
    m_rootDir   = dir;
    m_name      = name;
    m_entryFile = dir + "/src/main.ks";
    m_open      = true;

    return save();
}

bool ProjectManager::open(const QString& konprojPath) {
    QFile f(konprojPath);
    if (!f.open(QIODevice::ReadOnly)) return false;

    auto doc = QJsonDocument::fromJson(f.readAll());
    if (doc.isNull() || !doc.isObject()) return false;

    m_json    = doc.object();
    m_path    = konprojPath;
    m_rootDir = QFileInfo(konprojPath).absolutePath();
    m_name    = m_json.value("name").toString("Untitled");

    QString entry = m_json.value("entry").toString("src/main.ks");
    m_entryFile = m_rootDir + "/" + entry;
    m_open = true;
    return true;
}

bool ProjectManager::save() {
    if (!m_open) return false;
    QFile f(m_path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(m_json).toJson(QJsonDocument::Indented));
    return true;
}
