#include "widget.h"

Widget::Widget(QWidget *parent) : QWidget(parent) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setFixedSize(250, 250);

    cur_frame = 0;
    frame_delay = 100;

    moveAnim = new QPropertyAnimation(this, "pos");
    moveAnim->setDuration(200);
    moveAnim->setEasingCurve(QEasingCurve::OutQuad);

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
    img_w = 220;
    img_h = 220;

    QPainter painter(this);

    QPixmap oPixmap(":/resources/images/face/face_default.png");
    QPixmap sPixmap = oPixmap.scaled(img_w, img_h, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    img_lx = (width() - sPixmap.width()) / 2;
    img_ly = (height() - sPixmap.height()) / 2;

    painter.drawPixmap(img_lx, img_ly, sPixmap);
}

void Widget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (moveAnim->state() == QPropertyAnimation::Running) {
            moveAnim->stop();
        }
        mouseStartPos = event->globalPosition().toPoint();
        dragOffset = event->globalPosition().toPoint() - frameGeometry().topLeft();
        is_dragging = true;
        event->accept();
    }
}

void Widget::mouseMoveEvent(QMouseEvent* event) {
    if (is_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - dragOffset);
        event->accept();
    }
}

void Widget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        is_dragging = false;

        int min_dis = 0;

        QPoint releasePos = pos();
        QPoint targetPos = releasePos;

        // edge detection
        QScreen* screen = QApplication::primaryScreen();
        QRect screenRect = screen->geometry();

        if (releasePos.x() + img_lx < min_dis) {
            targetPos.setX(-img_lx);
        } else if (releasePos.x() > screenRect.width() + img_lx - width() - min_dis) {
            targetPos.setX(screenRect.width() + img_lx - width());
        }

        if (releasePos.y() + img_ly < min_dis) {
            targetPos.setY(-img_ly);
        } else if (releasePos.y() > screenRect.height() + img_ly - height() - min_dis) {
            targetPos.setY(screenRect.height() + img_ly - height());
        }

        if (targetPos != releasePos) {
            moveAnim->setStartValue(releasePos);
            moveAnim->setEndValue(targetPos);
            moveAnim->start();
        }

        event->accept();
    }
}
