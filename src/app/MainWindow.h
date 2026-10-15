#pragma once
#include <QFutureWatcher>
#include <QWidget>

#include <filesystem>
#include <vector>

#include "core/ExrFix.h"

class QLabel;
class QListWidget;
class QProgressBar;
class QPushButton;

// One drop zone: frames or folders dropped on it are destriped into a
// CleanedUp folder beside them, several frames at once on worker threads.
class MainWindow : public QWidget {
public:
    MainWindow();
    void addPaths(const std::vector<std::filesystem::path>& paths);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void startNext();
    void finished();
    void setHover(bool hover);

    QLabel* drop_;
    QListWidget* list_;
    QProgressBar* progress_;
    QPushButton* openFolder_;
    QFutureWatcher<p900::FixResult> watcher_;
    std::vector<std::filesystem::path> queue_, running_;
    std::filesystem::path lastFolder_;
    int done_ = 0, total_ = 0, failed_ = 0;
};
