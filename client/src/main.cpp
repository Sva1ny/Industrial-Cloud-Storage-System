#include <QApplication>
#include <QFont>
#include "MainWindow.h"
#include "styles/ThemeManager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("CloudDisk");
    app.setOrganizationName("CloudDisk");

    // Set system font with Chinese fallback
    QFont sysFont = app.font();
    sysFont.setFamilies({"PingFang SC", "Helvetica Neue", "Helvetica", "Arial"});
    sysFont.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(sysFont);

    // Apply global stylesheet
    app.setStyleSheet(ThemeManager::globalStyleSheet());

    MainWindow w;
    w.setWindowTitle("CloudDisk");
    w.show();

    return app.exec();
}
