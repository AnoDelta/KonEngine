#pragma once
#include <QDialog>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QMouseEvent>
#include <QPoint>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QEvent>

class WelcomeScreen : public QDialog {
    Q_OBJECT
public:
    explicit WelcomeScreen(QWidget* parent = nullptr);
    QString selectedProject() const { return m_selectedProject; }

signals:
    void projectChosen(const QString& path);

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    bool eventFilter(QObject* obj, QEvent* e) override;

private slots:
    void onNewProject();
    void onOpenProject();
    void onOpenFile();
    void onRecentDoubleClicked(QListWidgetItem* item);
    void onRemoveRecent();
    void onDeleteProject();
    void onOpenSelected();

private:
    void loadRecents();
    void addRecent(const QString& path);
    void updateEmptyState();

    QListWidget*  m_recentList    = nullptr;
    QPushButton*  m_trashBtn      = nullptr;
    QListWidgetItem* m_hoveredItem = nullptr;
    QString       m_selectedProject;
    QPoint        m_dragPos;
};
