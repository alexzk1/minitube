/* $BEGIN_LICENSE

This file is part of Minitube.
Copyright 2009, Flavio Tordini <flavio.tordini@gmail.com>

Minitube is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Minitube is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Minitube.  If not, see <http://www.gnu.org/licenses/>.

$END_LICENSE */

#include "videoareawidget.h"
#include "loadingwidget.h"
#include "mainwindow.h"
#include "playlistmodel.h"
#include "video.h"
#include "videomimedata.h"
#ifdef Q_OS_MAC
#include "macutils.h"
#endif
#include "snapshotpreview.h"
#include "fontutils.h"

namespace {

class MessageWidget : public QWidget {
public:
    MessageWidget(QWidget *parent = nullptr) : QWidget(parent) {
        QPalette p = palette();
        p.setColor(QPalette::Window, Qt::black);
        p.setColor(QPalette::WindowText, Qt::darkGray);
        p.setColor(QPalette::Base, Qt::black);
        p.setColor(QPalette::Text, Qt::darkGray);
        setPalette(p);
        setAutoFillBackground(true);

        QBoxLayout *l = new QHBoxLayout(this);
        l->setMargin(32);
        l->setSpacing(32);
        l->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        QLabel *arrowLabel = new QLabel("←");
        arrowLabel->setFont(FontUtils::light(64));
        arrowLabel->setPalette(p);
        l->addWidget(arrowLabel);

        QLabel *msgLabel = new QLabel(tr("Pick a video"));
        msgLabel->setFont(FontUtils::light(32));
        msgLabel->setPalette(p);
        l->addWidget(msgLabel);
    }
};
}

VideoAreaWidget::VideoAreaWidget(QWidget *parent)
    : QWidget(parent), videoWidget(0), messageWidget(0) {
    setAttribute(Qt::WA_OpaquePaintEvent);

    QBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);

    // hidden message widget
    messageLabel = new QLabel(this);
    messageLabel->setOpenExternalLinks(true);
    messageLabel->setMargin(7);
    messageLabel->setBackgroundRole(QPalette::ToolTipBase);
    messageLabel->setForegroundRole(QPalette::ToolTipText);
    messageLabel->setAutoFillBackground(true);
    messageLabel->setWordWrap(true);
    messageLabel->hide();
    layout->addWidget(messageLabel);

    stackedLayout = new QStackedLayout();
    layout->addLayout(stackedLayout);

#ifdef APP_SNAPSHOT
    snapshotPreview = new SnapshotPreview();
    connect(stackedLayout, SIGNAL(currentChanged(int)), snapshotPreview, SLOT(hide()));
#endif

    setAcceptDrops(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
            SLOT(showContextMenu(const QPoint &)));
}

void VideoAreaWidget::setVideoWidget(QWidget *videoWidget) {
    this->videoWidget = videoWidget;
    stackedLayout->addWidget(videoWidget);
}

void VideoAreaWidget::setLoadingWidget(LoadingWidget *loadingWidget) {
    this->loadingWidget = loadingWidget;
    stackedLayout->addWidget(loadingWidget);
    stackedLayout->setCurrentWidget(loadingWidget);
}

void VideoAreaWidget::showVideo() {
    if (videoWidget) stackedLayout->setCurrentWidget(videoWidget);
    loadingWidget->clear();
}

void VideoAreaWidget::showError(const QString &message) {
    messageLabel->setText(message);
    messageLabel->show();
    stackedLayout->setCurrentWidget(loadingWidget);
}

void VideoAreaWidget::showPickMessage() {
    if (!messageWidget) {
        messageWidget = new MessageWidget();
        stackedLayout->addWidget(messageWidget);
    }
    stackedLayout->setCurrentWidget(messageWidget);
}

void VideoAreaWidget::showLoading(Video *video) {
    messageLabel->hide();
    messageLabel->clear();
    stackedLayout->setCurrentWidget(loadingWidget);
    loadingWidget->setVideo(video);
}

#ifdef APP_SNAPSHOT
void VideoAreaWidget::showSnapshotPreview(const QPixmap &pixmap) {
    bool soundOnly = false;
#ifdef APP_MAC
    soundOnly = MainWindow::instance()->isReallyFullScreen();
#endif
    snapshotPreview->start(videoWidget, pixmap, soundOnly);
}

void VideoAreaWidget::hideSnapshotPreview() {}
#endif

void VideoAreaWidget::clear() {
    loadingWidget->clear();
    messageLabel->hide();
    messageLabel->clear();
    stackedLayout->setCurrentWidget(loadingWidget);
}

void VideoAreaWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) emit doubleClicked();
}

void VideoAreaWidget::dragEnterEvent(QDragEnterEvent *event) {
    // qDebug() << event->mimeData()->formats();
    if (event->mimeData()->hasFormat("application/x-minitube-video")) {
        event->acceptProposedAction();
    }
}

void VideoAreaWidget::dropEvent(QDropEvent *event) {
    const VideoMimeData *videoMimeData = qobject_cast<const VideoMimeData *>(event->mimeData());
    if (!videoMimeData) return;

    QVector<Video *> droppedVideos = videoMimeData->getVideos();
    if (droppedVideos.isEmpty()) return;
    Video *video = droppedVideos.at(0);
    int row = listModel->rowForVideo(video);
    if (row != -1) listModel->setActiveRow(row);
    event->acceptProposedAction();
}

void VideoAreaWidget::showContextMenu(const QPoint &point) {
    QMenu *menu = MainWindow::instance()->getMenu("video");
    menu->exec(mapToGlobal(point));
}
