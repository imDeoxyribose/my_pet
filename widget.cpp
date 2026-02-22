#include "widget.h"

Widget::Widget(QWidget *parent) : QWidget(parent) {
    // window
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setFixedSize(300, 300);

    // face
    img_w = 220;
    img_h = 220;

    QPixmap o_facePixmap(":/resources/images/face/face_default.png");
    facePixmap = o_facePixmap.scaled(img_w, img_h, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    img_lx = (width() - facePixmap.width()) / 2;
    img_ly = (height() - facePixmap.height()) / 2;

    // eyes
    float k = 0.7; // eyes proportionality factor
    eyes_w = img_w * k;
    eyes_h = img_h * k / 2;

    QPixmap o_eyesPixmap(":/resources/images/eyes/eyes_default.png");
    eyesPixmap = o_eyesPixmap.scaled(eyes_w, eyes_h, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    eyesBaseCenterPos = QPointF(img_lx + img_w * 0.5, img_ly + img_h * 0.4);
    eyesCurCenterPos = eyesBaseCenterPos;
    maxOffset = 15;
    sensitivity = 0.02; // eyes sensitivity factor

    eyesTimer = new QTimer(this);
    connect(eyesTimer, &QTimer::timeout, this, &Widget::updateMousePos);
    eyesTimer->start(50);

    globalMousePos = QCursor::pos();

    // animation
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
    QPainter painter(this);

    painter.drawPixmap(img_lx, img_ly, facePixmap);

    calculateEyesPos();

    painter.drawPixmap(toEyesCenterAlignedPos(eyesCurCenterPos).toPoint(), eyesPixmap);
    //test
    painter.drawRect(img_lx, img_ly, img_w, img_h);
    painter.drawRect(0, 0, width(), height());
    painter.drawRect(toEyesCenterAlignedPos(eyesCurCenterPos).x(), toEyesCenterAlignedPos(eyesCurCenterPos).y(), eyes_w, eyes_h);
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

        int min_dis = -30;

        QPoint releasePos = pos();
        QPoint targetPos = releasePos;

        // edge detection
        QScreen* screen = QApplication::primaryScreen();
        QRect screenRect = screen->geometry();

        if (releasePos.x() + img_lx < min_dis) {
            targetPos.setX(min_dis - img_lx);
        } else if (releasePos.x() + img_lx + img_w > screenRect.width() - min_dis) {
            targetPos.setX(screenRect.width() - min_dis - img_w - img_lx);
        }

        if (releasePos.y() + img_ly < min_dis) {
            targetPos.setY(min_dis - img_ly);
        } else if (releasePos.y() + img_ly + img_h > screenRect.height() - min_dis) {
            targetPos.setY(screenRect.height() - min_dis - img_h - img_ly);
        }

        if (targetPos != releasePos) {
            moveAnim->setStartValue(releasePos);
            moveAnim->setEndValue(targetPos);
            moveAnim->start();
        }

        event->accept();
    }
}

QPointF Widget::toEyesCenterAlignedPos(QPointF pos) {
    pos.setX(pos.x() - eyes_w / 2);
    pos.setY(pos.y() - eyes_h / 2);
    return pos;
}

void Widget::updateMousePos() {
    QPoint newGlobalMousePos = QCursor::pos();
    if (newGlobalMousePos != globalMousePos) {
        globalMousePos = newGlobalMousePos;
        update();
    }
}

void Widget::calculateEyesPos() {
    QPoint windowPos = this->pos();
    QSize windowSize = this->size();
    QPointF windowCenter = QPointF(windowPos.x() + windowSize.width() / 2, windowPos.y() + windowSize.height() / 2);
    QPointF dir = globalMousePos - windowCenter;
    float dis = std::sqrt(dir.x() * dir.x() + dir.y() * dir.y());

    if (dis < 1.0f) {
        eyesCurCenterPos = eyesBaseCenterPos;
        return;
    }

    float nX = dir.x() / dis;
    float nY = dir.y() / dis;
    float offsetAmount = std::min(dis * sensitivity, maxOffset);
    QPointF offset = QPointF(nX * offsetAmount, nY * offsetAmount);
    eyesCurCenterPos = eyesBaseCenterPos + offset;
}

