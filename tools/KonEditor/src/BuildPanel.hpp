#pragma once
#include <QWidget>
#include <QPlainTextEdit>
#include <QTextCharFormat>
#include <QPushButton>
#include <QProcess>
#include <QLabel>
#include <QElapsedTimer>
#include <QProgressBar>
#include <QTimer>
#include <QFileInfo>

class BuildPanel : public QWidget {
    Q_OBJECT
public:
    explicit BuildPanel(QWidget* parent = nullptr);

    void build(const QString& entryFile, const QString& target,
               const QString& outDir, bool runAfter = false);
    void appendLog(const QString& text);

signals:
    void buildRequested();
    void runRequested();
    void gameStarted(QProcess* proc);

private slots:
    void onProcessOutput();
    void onProcessError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus);

private:
    QPlainTextEdit* m_log       = nullptr;
    QLabel*         m_status    = nullptr;
    QProgressBar*   m_progress  = nullptr;
    QTimer*         m_elapsedTimer  = nullptr;
    QTimer*         m_buildTimeout  = nullptr;
    QPushButton*    m_cancelBtn = nullptr;
    QProcess*       m_proc      = nullptr;
    QElapsedTimer   m_timer;

    QString m_entryFile;
    QString m_outDir;
    QString m_target;
    QString m_konscript;
    bool    m_runAfter = false;

    void startBuild();
    void runGame();
};
