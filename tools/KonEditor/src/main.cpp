#include <QApplication>
#include "KonEditor.hpp"
#include "WelcomeScreen.hpp"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("KonEditor");
    app.setOrganizationName("AnoDelta");
    app.setApplicationVersion("0.1.0");

    // Dark theme before welcome screen
    qApp->setStyle("Fusion");
    QPalette dark;
    dark.setColor(QPalette::Window,          QColor(30, 30, 30));
    dark.setColor(QPalette::WindowText,      QColor(220, 220, 220));
    dark.setColor(QPalette::Base,            QColor(22, 22, 22));
    dark.setColor(QPalette::AlternateBase,   QColor(35, 35, 35));
    dark.setColor(QPalette::Text,            QColor(220, 220, 220));
    dark.setColor(QPalette::Button,          QColor(45, 45, 45));
    dark.setColor(QPalette::ButtonText,      QColor(220, 220, 220));
    dark.setColor(QPalette::Highlight,       QColor(0, 120, 215));
    dark.setColor(QPalette::HighlightedText, Qt::white);
    qApp->setPalette(dark);

    // Show welcome screen
    WelcomeScreen welcome;
    if (welcome.exec() != QDialog::Accepted)
        return 0; // user closed welcome screen

    KonEditor editor;
    editor.show();

    QString project = welcome.selectedProject();
    if (!project.isEmpty())
        editor.openProject(project);

    return app.exec();
}
