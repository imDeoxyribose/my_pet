#include "petController.h"
#include "petWindow.h"
#include <QDebug>
#include <QtMath>
#include <QEasingCurve>
#include <QShortcut>

PetController::PetController(PetWindow* parentWindow, QObject *parent)
    : QObject(parent), window(parentWindow), currentAnimationState(PetAnimationState::Idle) {
    img_w = 220;
    img_h = 220;

    QPixmap o_facePixmap(":/resources/images/face/face_default.png");
    facePixmap = o_facePixmap.scaled(img_w, img_h, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    img_lx = (window->getWindowRect().width() - facePixmap.width()) / 2;
    img_ly = (window->getWindowRect().height() - facePixmap.height()) / 2;

    idleMaxScaleX = 0.96f;
    idleMaxScaleY = 1.06f;
    sleepMaxScaleX = 1.05f;
    sleepMaxScaleY = 0.95f;
    m_currentScaleX = 1.0f;
    m_currentScaleY = 1.0f;
    idleAnimDuration = 1000;
    sleepAnimDuration = 2000;

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

    scaleXAnimation = new QPropertyAnimation(this, "currentScaleX");
    scaleXAnimation->setDuration(1000);
    scaleXAnimation->setStartValue(1.0f);
    scaleXAnimation->setEndValue(idleMaxScaleX);
    scaleXAnimation->setEasingCurve(QEasingCurve::InOutSine);

    scaleYAnimation = new QPropertyAnimation(this, "currentScaleY");
    scaleYAnimation->setDuration(1000);
    scaleYAnimation->setStartValue(1.0f);
    scaleYAnimation->setEndValue(idleMaxScaleY);
    scaleYAnimation->setEasingCurve(QEasingCurve::InOutSine);

    connect(scaleXAnimation, &QPropertyAnimation::finished, this, [this]() {
        if (scaleXAnimation->direction() == QPropertyAnimation::Forward) {
            scaleXAnimation->setDirection(QPropertyAnimation::Backward);
        } else {
            scaleXAnimation->setDirection(QPropertyAnimation::Forward);
        }
        scaleXAnimation->start();
    });

    connect(scaleYAnimation, &QPropertyAnimation::finished, this, [this]() {
        if (scaleYAnimation->direction() == QPropertyAnimation::Forward) {
            scaleYAnimation->setDirection(QPropertyAnimation::Backward);
        } else {
            scaleYAnimation->setDirection(QPropertyAnimation::Forward);
        }
        scaleYAnimation->start();
    });

    startAnimation(PetAnimationState::Idle);

    QShortcut* shortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    shortcut->setContext(Qt::ApplicationShortcut);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        if (currentAnimationState == PetAnimationState::Idle) {
            setAnimationState(PetAnimationState::Sleep);
        } else if (currentAnimationState == PetAnimationState::Sleep) {
            setAnimationState(PetAnimationState::Idle);
        }
    });
}

PetController::~PetController() {
    stopAnimation(PetAnimationState::Idle);
}

QRect PetController::getImageRect() const {
    return QRect(img_lx, img_ly, img_w, img_h);
}

void PetController::render(QPainter* painter) {
    if (currentAnimationState == PetAnimationState::None) {
        painter->drawPixmap(img_lx, img_ly, facePixmap);
    } else if (currentAnimationState == PetAnimationState::Idle || currentAnimationState == PetAnimationState::Sleep) {
        painter->save();
        
        float centerX = img_lx + img_w / 2.0f;
        float centerY = img_ly + img_h / 2.0f;
        
        painter->translate(centerX, centerY);
        painter->scale(m_currentScaleX, m_currentScaleY);
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
        PetAnimationState oldState = currentAnimationState;
        currentAnimationState = state;
        
        if (oldState == PetAnimationState::Idle) {
            stopAnimation(PetAnimationState::Idle);
        }
        if (oldState == PetAnimationState::Sleep) {
            stopAnimation(PetAnimationState::Sleep);
        }
        
        if (currentAnimationState == PetAnimationState::Idle) {
            startAnimation(PetAnimationState::Idle);
        }
        if (currentAnimationState == PetAnimationState::Sleep) {
            startAnimation(PetAnimationState::Sleep);
        }
        
        if (window) {
            window->update();
        }
    }
}

PetAnimationState PetController::getAnimationState() const {
    return currentAnimationState;
}

void PetController::setCurrentScaleX(float scale) {
    if (qAbs(m_currentScaleX - scale) > 0.001f) {
        m_currentScaleX = scale;
        emit currentScaleXChanged();
        if (window) {
            window->update();
        }
    }
}

void PetController::setCurrentScaleY(float scale) {
    if (qAbs(m_currentScaleY - scale) > 0.001f) {
        m_currentScaleY = scale;
        emit currentScaleYChanged();
        if (window) {
            window->update();
        }
    }
}

void PetController::startAnimation(PetAnimationState state) {
    if (state == PetAnimationState::Idle) {
        scaleXAnimation->setDuration(idleAnimDuration);
        scaleYAnimation->setDuration(idleAnimDuration);
        scaleXAnimation->setEndValue(idleMaxScaleX);
        scaleYAnimation->setEndValue(idleMaxScaleY);
        if (scaleXAnimation->state() != QPropertyAnimation::Running) {
            scaleXAnimation->start();
        }
        if (scaleYAnimation->state() != QPropertyAnimation::Running) {
            scaleYAnimation->start();
        }
    } else if (state == PetAnimationState::Sleep) {
        scaleXAnimation->setDuration(sleepAnimDuration);
        scaleYAnimation->setDuration(sleepAnimDuration);
        scaleXAnimation->setEndValue(sleepMaxScaleX);
        scaleYAnimation->setEndValue(sleepMaxScaleY);
        if (scaleXAnimation->state() != QPropertyAnimation::Running) {
            scaleXAnimation->start();
        }
        if (scaleYAnimation->state() != QPropertyAnimation::Running) {
            scaleYAnimation->start();
        }
    }
}

void PetController::stopAnimation(PetAnimationState state) {
    if (scaleXAnimation->state() == QPropertyAnimation::Running) {
        scaleXAnimation->stop();
    }
    if (scaleYAnimation->state() == QPropertyAnimation::Running) {
        scaleYAnimation->stop();
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
