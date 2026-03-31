#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include <QDockWidget>
#include <QListWidget>
#include <QProcess>
#include <QLabel>
#include <QTimer>
#include <QMap>
#include <QString>
#include "KonScriptEditor.hpp"

// -----------------------------------------------------------------------
// KonScriptIDE
// -----------------------------------------------------------------------
class KonScriptIDE : public QMainWindow {
    Q_OBJECT
public:
    explicit KonScriptIDE(QWidget* parent = nullptr);

    void openFile(const QString& path);
    KonScriptEditor* currentEditor();
    void setBuildTarget(const QString& target) { m_buildTarget = target; }

signals:
    void fileOpened(const QString& path);
    void fileSaved(const QString& path);

private slots:
    void newFile();
    void openFileDialog();
    bool saveCurrentFile();
    bool saveCurrentFileAs();
    void closeTab(int index);

    void runCurrent();
    void buildCurrent();
    void checkCurrent();

    void onProcessOutput();
    void onProcessError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

    void onTabChanged(int index);
    void onTextModified();
    void runBackgroundCheck();

private:
    void setupUI();
    void setupToolbar();
    void setupDocks();
    void applyTheme();

    // Widget-pointer helpers — immune to index shifts
    QString  pathFor(KonScriptEditor* ed) const    { return m_paths.value(ed); }
    bool     isModified(KonScriptEditor* ed) const { return m_modified.value(ed, false); }
    void     setPath(KonScriptEditor* ed, const QString& p) { m_paths[ed] = p; }
    void     setModified(KonScriptEditor* ed, bool v)       { m_modified[ed] = v; }

    KonScriptEditor* editorAt(int index) const {
        return qobject_cast<KonScriptEditor*>(m_tabs->widget(index));
    }

    void updateTabTitle(KonScriptEditor* ed);
    void parseErrorOutput(const QString& output);
    void appendOutput(const QString& text, bool isError = false);
    void clearOutput();
    int  findTabForPath(const QString& path) const;
    bool isBlankUntitled(KonScriptEditor* ed) const;
    KonScriptEditor* addEditorTab(const QString& title);

    // ── Widgets ────────────────────────────────────────────────────────
    QTabWidget*  m_tabs       = nullptr;
    QDockWidget* m_outputDock = nullptr;
    QListWidget* m_errorList  = nullptr;
    QLabel*      m_statusBar  = nullptr;
    QLabel*      m_cursorPos  = nullptr;

    // ── State — keyed by widget pointer, never by index ────────────────
    QMap<KonScriptEditor*, QString> m_paths;
    QMap<KonScriptEditor*, bool>    m_modified;

    QProcess* m_process      = nullptr;
    QProcess* m_checkProcess = nullptr;
    QTimer*   m_checkTimer   = nullptr;
    QString   m_buildTarget  = "linux64";
    int       m_untitledCount = 0;
};
