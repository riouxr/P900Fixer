#include "MainWindow.h"

#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

namespace {
void applyDarkTheme(QApplication& app) {
    app.setStyle(QStyleFactory::create("Fusion"));
    QPalette p;
    const QColor window(38, 38, 38), base(30, 30, 30), text(210, 210, 210), button(52, 52, 52);
    p.setColor(QPalette::Window, window);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, base);
    p.setColor(QPalette::AlternateBase, window);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::Button, button);
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::ToolTipBase, button);
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::Highlight, QColor(80, 80, 80));
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(110, 110, 110));
    p.setColor(QPalette::Disabled, QPalette::Text, QColor(110, 110, 110));
    app.setPalette(p);
    app.setStyleSheet("QListWidget, QProgressBar { border: 1px solid #4a4a4a; }"
                      "QProgressBar { text-align: center; background: #1e1e1e; }"
                      "QProgressBar::chunk { background: #4f6f4f; }"
                      "QToolTip { border: 1px solid #5a5a5a; }");
}
}  // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    applyDarkTheme(app);
    MainWindow window;
    window.show();

    // Frames dropped on the .exe's icon arrive as arguments.
    std::vector<std::filesystem::path> paths;
    const QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i) paths.emplace_back(args[i].toStdWString());
    if (!paths.empty()) window.addPaths(paths);
    return app.exec();
}
