#include "MainWindow.h"

#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QLabel>
#include <QListWidget>
#include <QMimeData>
#include <QProgressBar>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentMap>

namespace fs = std::filesystem;

namespace {
QString qpath(const fs::path& p) { return QString::fromStdWString(p.wstring()); }

const char* kDropStyle =
    "QLabel { border: 2px dashed #5a5a5a; border-radius: 10px; color: #b4b4b4;"
    " font-size: 15px; background: #262626; padding: 24px; }";
const char* kDropHover =
    "QLabel { border: 2px dashed #8fb3d9; border-radius: 10px; color: #e0e0e0;"
    " font-size: 15px; background: #2d3540; padding: 24px; }";
}  // namespace

MainWindow::MainWindow() {
    setWindowTitle("P900 Fixer");
    setAcceptDrops(true);
    resize(640, 520);

    drop_ = new QLabel("Drop EXR frames or folders here\n\n"
                       "The vertical column lines are removed and the frames\n"
                       "are written to a CleanedUp folder beside them.");
    drop_->setAlignment(Qt::AlignCenter);
    drop_->setMinimumHeight(180);
    drop_->setStyleSheet(kDropStyle);

    list_ = new QListWidget;
    list_->setUniformItemSizes(true);
    progress_ = new QProgressBar;
    progress_->setFormat("%v / %m");
    progress_->setRange(0, 1);
    progress_->setValue(0);
    openFolder_ = new QPushButton("Open CleanedUp folder");
    openFolder_->setEnabled(false);
    connect(openFolder_, &QPushButton::clicked, this,
            [this] { QDesktopServices::openUrl(QUrl::fromLocalFile(qpath(lastFolder_))); });

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(drop_);
    layout->addWidget(list_, 1);
    layout->addWidget(progress_);
    layout->addWidget(openFolder_);

    connect(&watcher_, &QFutureWatcher<p900::FixResult>::resultReadyAt, this, [this](int index) {
        const p900::FixResult r = watcher_.resultAt(index);
        const fs::path& input = running_[index];
        auto* item = new QListWidgetItem(QString("%1   %2  %3")
                                             .arg(r.ok ? "OK " : "ERR")
                                             .arg(qpath(input.filename()))
                                             .arg(QString::fromStdString(r.message)));
        item->setForeground(r.ok ? QColor(170, 200, 160) : QColor(230, 120, 110));
        item->setToolTip(qpath(r.ok ? r.output : input));
        list_->addItem(item);
        list_->scrollToBottom();
        if (r.ok) lastFolder_ = r.output.parent_path(); else ++failed_;
        progress_->setValue(++done_);
    });
    connect(&watcher_, &QFutureWatcher<p900::FixResult>::finished, this, [this] { finished(); });
}

void MainWindow::addPaths(const std::vector<fs::path>& paths) {
    std::vector<fs::path> frames = p900::collectFrames(paths);
    if (frames.empty()) {
        list_->addItem("Nothing to do: drop .exr files or folders holding them.");
        return;
    }
    queue_.insert(queue_.end(), frames.begin(), frames.end());
    total_ += (int)frames.size();
    progress_->setRange(0, total_);
    if (!watcher_.isRunning()) startNext();
}

void MainWindow::startNext() {
    if (queue_.empty()) return;
    running_ = std::move(queue_);
    queue_.clear();
    watcher_.setFuture(QtConcurrent::mapped(running_, [](const fs::path& p) { return p900::fixExr(p); }));
}

void MainWindow::finished() {
    openFolder_->setEnabled(!lastFolder_.empty());
    if (!queue_.empty()) {
        startNext();
        return;
    }
    list_->addItem(QString("Done: %1 frame%2, %3 failed.").arg(done_).arg(done_ == 1 ? "" : "s").arg(failed_));
    list_->scrollToBottom();
    done_ = total_ = failed_ = 0;
}

void MainWindow::setHover(bool hover) { drop_->setStyleSheet(hover ? kDropHover : kDropStyle); }

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        setHover(true);
    }
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent*) { setHover(false); }

void MainWindow::dropEvent(QDropEvent* event) {
    setHover(false);
    std::vector<fs::path> paths;
    for (const QUrl& url : event->mimeData()->urls())
        if (url.isLocalFile()) paths.emplace_back(url.toLocalFile().toStdWString());
    event->acceptProposedAction();
    addPaths(paths);
}
