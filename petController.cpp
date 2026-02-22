#include "petController.h"
#include "petWindow.h"

PetController::PetController(PetWindow* parentWindow, QObject *parent)
    : QObject(parent), window(parentWindow) {

    // face
    img_w = 220;
    img_h = 220;

    QPixmap o_facePixmap(":/resources/images/face/face_default.png");
    facePixmap = o_facePixmap.scaled(img_w, img_h, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    img_lx = (window->getWindowRect().width() - facePixmap.width()) / 2;
    img_ly = (window->getWindowRect().height() - facePixmap.height()) / 2;

    // eyes
    enable_EyesFollowing = true;

    float k = 0.7;
    eyes_w = img_w * k;
    eyes_h = img_h * k / 2;

    QPixmap o_eyesPixmap(":/resources/images/eyes/eyes_default.png");
    eyesPixmap = o_eyesPixmap.scaled(eyes_w, eyes_h, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    eyesBaseCenterPos = QPointF(img_lx + img_w * 0.5, img_ly + img_h * 0.4);
    eyesCurCenterPos = eyesBaseCenterPos;
    maxOffset = 15;
    sensitivity = 0.02;

    if (enable_EyesFollowing) {
        eyesTimer = new QTimer(this);
        connect(eyesTimer, &QTimer::timeout, this, &PetController::updateMousePos);
        eyesTimer->start(50);
    }

    globalMousePos = QCursor::pos();
}

PetController::~PetController() {}

QRect PetController::getImageRect() const {
    return QRect(img_lx, img_ly, img_w, img_h);
}

void PetController::render(QPainter* painter) {
    painter->drawPixmap(img_lx, img_ly, facePixmap);

    calculateEyesPos();

    painter->drawPixmap(toEyesCenterAlignedPos(eyesCurCenterPos).toPoint(), eyesPixmap);
    painter->drawRect(img_lx, img_ly, img_w, img_h);
    painter->drawRect(toEyesCenterAlignedPos(eyesCurCenterPos).x(), toEyesCenterAlignedPos(eyesCurCenterPos).y(), eyes_w, eyes_h);
}

QPointF PetController::toEyesCenterAlignedPos(QPointF pos) {
    pos.setX(pos.x() - eyes_w / 2);
    pos.setY(pos.y() - eyes_h / 2);
    return pos;
}

void PetController::updateMousePos() {
    QPoint newGlobalMousePos = QCursor::pos();
    if (newGlobalMousePos != globalMousePos) {
        globalMousePos = newGlobalMousePos;
        if (window) {
            window->update();
        }
    }
}

void PetController::calculateEyesPos() {
    QPoint windowPos = window->pos();
    QSize windowSize = window->size();
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

    static QPointF smoothedPos = eyesBaseCenterPos;
    smoothedPos += (eyesBaseCenterPos - smoothedPos) * 0.3;
    eyesBaseCenterPos = smoothedPos;
}

