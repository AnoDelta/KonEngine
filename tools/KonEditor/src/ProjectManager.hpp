#pragma once
#include <QObject>
#include <QString>
#include <QJsonObject>

class ProjectManager : public QObject {
    Q_OBJECT
public:
    explicit ProjectManager(QObject* parent = nullptr);

    bool create(const QString& dir);
    bool open(const QString& konprojPath);
    bool save();
    bool isOpen() const { return m_open; }

    QString name()      const { return m_name; }
    QString path()      const { return m_path; }       // path to .konproj
    QString rootDir()   const { return m_rootDir; }    // project folder
    QString entryFile() const { return m_entryFile; }  // src/main.ks
    QString outDir()    const { return m_rootDir + "/build"; }
    QJsonObject json()  const { return m_json; }
    void setJson(const QJsonObject& j) { m_json = j; }

private:
    bool     m_open     = false;
    QString  m_name;
    QString  m_path;
    QString  m_rootDir;
    QString  m_entryFile;
    QJsonObject m_json;
};
