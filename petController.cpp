#include "petController.h"
#include "petWindow.h"
#include <QDebug>
#include <QtMath>

PetController::PetController(PetWindow* parentWindow, QObject *parent)
    : QObject(parent), window(parentWindow), currentAnimationState(PetAnimationState::Idle), cur_frame(0) {
    img_w = 220;
    img_h = 220;

    QPixmap o_facePixmap(":/resources/images/face/face_default.png");
    facePixmap = o_facePixmap.scaled(img_w, img_h, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    img_lx = (window->getWindowRect().width() - facePixmap.width()) / 2;
    img_ly = (window->getWindowRect().height() - facePixmap.height()) / 2;

    maxScaleX = 0.96f;
    maxScaleY = 1.06f;
    currentScaleX = 1.0f;
    currentScaleY = 1.0f;
    scaleProgress = 0.0f;
    isScalingUp = true;

    enable_EyesFollowing = true;
    maxOffset = 15;
    sensitivity = 0.02;
    float k = 0.7;
    eyes_w = img_w * k;
    eyes_h = img_h * k / 2;
    eyesBaseCenterPos = QPointF(img_lx + img_w * 0.5, img_ly + img_h * 0.4);
    eyesCurCenterPos = eyesBaseCenterPos;
    globalMousePos = QCursor::pos();

    QPixmap o_eyesPixmap(":/resources/images/eyes/eyes_default.png");
    eyesPixmap = o_eyesPixmap.scaled(eyes_w, eyes_h, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    if (enable_EyesFollowing) {
        eyesTimer = new QTimer(this);
        connect(eyesTimer, &QTimer::timeout, this, &PetController::updateMousePos);
        eyesTimer->start(50);
    }

    frame_delay = 30;
    animTimer = new QTimer(this);
    connect(animTimer, &QTimer::timeout, this, &PetController::updateAnimation);
    animTimer->start(frame_delay);
}

PetController::~PetController() {}

QRect PetController::getImageRect() const {
    return QRect(img_lx, img_ly, img_w, img_h);
}

void PetController::render(QPainter* painter) {
    if (currentAnimationState == PetAnimationState::None) {
        painter->drawPixmap(img_lx, img_ly, facePixmap);
    } else if (currentAnimationState == PetAnimationState::Idle) {
        painter->save();
        
        float centerX = img_lx + img_w / 2.0f;
        float centerY = img_ly + img_h / 2.0f;
        
        painter->translate(centerX, centerY);
        painter->scale(currentScaleX, currentScaleY);
        painter->translate(-centerX, -centerY);
        
        painter->drawPixmap(img_lx, img_ly, facePixmap);
        
        painter->restore();
    }

    if (enable_EyesFollowing) {
        calculateEyesPos();
        painter->drawPixmap(toEyesCenterAlignedPos(eyesCurCenterPos).toPoint(), eyesPixmap);
    }
}

void PetController::setAnimationState(PetAnimationState state) {
    if (currentAnimationState != state) {
        currentAnimationState = state;
        //cur_frame = 0;
        scaleProgress = 0.0f;
        currentScaleX = 1.0f;
        currentScaleY = 1.0f;
        isScalingUp = true;
        if (window) {
            window->update();
        }
    }
}

PetAnimationState PetController::getAnimationState() const {
    return currentAnimationState;
}

void PetController::updateAnimation() {
    if (currentAnimationState == PetAnimationState::Idle) {
        float scaleStep = 0.02f;
        
        if (isScalingUp) {
            scaleProgress += scaleStep;
            if (scaleProgress >= 1.0f) {
                scaleProgress = 1.0f;
                isScalingUp = false;
            }
        } else {
            scaleProgress -= scaleStep;
            if (scaleProgress <= 0.0f) {
                scaleProgress = 0.0f;
                isScalingUp = true;
            }
        }
        
        float easedProgress = (1.0f - M_PI_2 + M_PI * scaleProgress) / 2.0f;
        currentScaleX = 1.0f + (maxScaleX - 1.0f) * easedProgress;
        currentScaleY = 1.0f + (maxScaleY - 1.0f) * easedProgress;
        
        if (window) {
            window->update();
        }
    }
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
