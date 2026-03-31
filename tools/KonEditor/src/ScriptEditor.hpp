#pragma once
#include <QWidget>
#include <QTabWidget>
#include <QMap>
#include <QSet>
// Use KonScriptIDE's editor widget directly
#include "../../KonScriptIDE/src/KonScriptEditor.hpp"

class ScriptEditor : public QWidget {
    Q_OBJECT
public:
    explicit ScriptEditor(QWidget* parent = nullptr);
    void setProjectRoot(const QString& root);
    void openFile(const QString& path);
    void reloadFile(const QString& path);
    void saveAll();
    void saveCurrentFile();
    void highlightErrors(const QStringList& errorLines);

private slots:
    void onTabCloseRequested(int index);
    void onTextChanged();

private:
    QTabWidget* m_tabs        = nullptr;
    QString     m_projectRoot;

    struct FileTab {
        QString path;
        bool    modified = false;
    };
    QMap<int, FileTab> m_fileTabs;

    KonScriptEditor* editorAt(int index);
    void updateTabTitle(int index);
};
