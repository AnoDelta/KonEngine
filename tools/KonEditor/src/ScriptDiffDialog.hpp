#pragma once
#include <QDialog>
#include <QList>
#include "SceneFormat.hpp"

// Shows detected script changes as diffs and lets user accept/reject each
class ScriptDiffDialog : public QDialog {
    Q_OBJECT
public:
    explicit ScriptDiffDialog(const QList<ScriptChange>& changes,
                               const QString& projectRoot,
                               QWidget* parent = nullptr);
    void applyAccepted();

private:
    QList<ScriptChange> m_changes;
    QList<bool>         m_accepted;
    QString             m_projectRoot;

    QString makeDiff(const ScriptChange& c);
};
