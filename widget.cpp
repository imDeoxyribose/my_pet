#include "widget.h"

Widget::Widget(QWidget *parent) : QWidget(parent) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setFixedSize(250, 250);

    cur_frame = 0;
    frame_delay = 100;

    loadIdleAnimation();
}

Widget::~Widget() {}

void Widget::loadIdleAnimation() {
    idleFrames.clear();
    for (int i = 0; i <=6; i++) {
        QString path = QString(":/resources/images/face/idle/%1.png").arg(i);
        QPixmap pixmap(path);
        if (!pixmap.isNull()) {
            idleFrames.append(pixmap);
        } else {
            qDebug() << "can't load frame " << i << " path:" << path;
        }
    }
}

void Widget::paintEvent(QPaintEvent *event)
{
    int imgW = 220;
    int imgH = 220;

    QPainter painter(this);

    QPixmap oPixmap(":/resources/images/face/face_default.png");
    QPixmap sPixmap = oPixmap.scaled(imgW, imgH, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    int x = (width() - sPixmap.width()) / 2;
    int y = (height() - sPixmap.height()) / 2;

    painter.drawPixmap(x, y, sPixmap);
}

void Widget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
        is_dragging = true;
        event->accept();
    }
}

void Widget::mouseMoveEvent(QMouseEvent* event) {
    if (is_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - dragPos);
        event->accept();
    }
}

void Widget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        is_dragging = false;
        event->accept();
    }
}
