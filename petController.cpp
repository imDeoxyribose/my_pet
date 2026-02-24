#include "petController.h"
#include "petWindow.h"
#include <QDebug>
#include <QtMath>
#include <QEasingCurve>
#include <QShortcut>

PetController::PetController(PetWindow* parentWindow, QObject *parent)
    : QObject(parent), window(parentWindow), curAnimationState(PetAnimationState::None) {

    // face setup
    img_w = 220;
    img_h = 220;
    img_lx = (window->getWindowRect().width() - img_w) / 2;
    img_ly = (window->getWindowRect().height() - img_h) / 2;
    QPixmap o_facePixmap(":/resources/images/face/face_default.png");
    facePixmap = o_facePixmap.scaled(img_w, img_h, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // face idle animation
    scaleXAnimation= new QPropertyAnimation(this, "curScaleX");
    scaleYAnimation= new QPropertyAnimation(this, "curScaleY");
    m_curScaleX = 1.0f;
    m_curScaleY = 1.0f;
    idleMaxScaleX = 0.96f;
    idleMaxScaleY = 1.06f;
    idleAnimDuration = 1000;

    // face sleep animation
    sleepMaxScaleX = 1.05f;
    sleepMaxScaleY = 0.95f;
    sleepAnimDuration = 2000;

    // face sleep transition animation
    isPlayingFaceSleepTransition = false;

    // eyes setup
    float k = 0.7f;
    eyes_w = img_w * k;
    eyes_h = img_h * k / 2;
    QPixmap o_eyesPixmap(":/resources/images/eyes/eyes_default.png");
    eyesPixmap = o_eyesPixmap.scaled(eyes_w, eyes_h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    eyesCenterPos = QPointF(img_lx + img_w * 0.5, img_ly + img_h * 0.4);
    eyesSmoothedCenterPos = eyesCenterPos;
    eyesCurCenterPos = eyesCenterPos;

    // eyes following
    isEyesFollowing = false;
    maxOffset = 15.0f;
    sensitivity = 0.02f;
    globalMousePos = QCursor::pos();
    eyesFollowTimer = new QTimer(this);

    // eyes homing animation
    isPlayingEyesHoming = false;
    eyesHomingAnimation = new QPropertyAnimation(this, "curEyesPos");

    // eyes sleep transition animation
    isPlayingEyesSleepTransition = false;
    sleepFrameIndex = 0;
    sleepFrameTimer = new QTimer(this);

    // connection setup
    setupConnection();

    loadSleepFrames();

    // initialization
    setAnimationState(PetAnimationState::Idle);

    //temp
    QShortcut* shortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    shortcut->setContext(Qt::ApplicationShortcut);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        if (curAnimationState == PetAnimationState::Idle) {
            setAnimationState(PetAnimationState::Sleep);
        } else if (curAnimationState == PetAnimationState::Sleep) {
            setAnimationState(PetAnimationState::Idle);
        }
    });
}

PetController::~PetController() {
    setAnimationState(PetAnimationState::None);
}

QRect PetController::getImageRect() const {
    return QRect(img_lx, img_ly, img_w, img_h);
}

void PetController::render(QPainter* painter) {

    // render face
    if (curAnimationState == PetAnimationState::None) {
        painter->drawPixmap(img_lx, img_ly, facePixmap);
    } else if (curAnimationState == PetAnimationState::Idle || curAnimationState == PetAnimationState::Sleep) {
        float centerX = img_lx + img_w / 2.0f;
        float centerY = img_ly + img_h / 2.0f;
        painter->save();
        painter->translate(centerX, centerY);
        painter->scale(m_curScaleX, m_curScaleY);
        painter->translate(-centerX, -centerY);
        painter->drawPixmap(img_lx, img_ly, facePixmap);
        painter->restore();
    }

    // render eyes
    if (curAnimationState == PetAnimationState::None) {
        painter->drawPixmap(toEyesCenterAlignedPos(eyesCenterPos).toPoint(), eyesPixmap);
    } else {
        if (isEyesFollowing) {
            updateEyesPos();
        }
        painter->drawPixmap(toEyesCenterAlignedPos(eyesCurCenterPos).toPoint(), eyesPixmap);
    }
}

void PetController::setAnimationState(PetAnimationState state) {
    if (curAnimationState != state) {
        PetAnimationState oldState = curAnimationState;
        curAnimationState = state;
        
        if (oldState == PetAnimationState::Idle) {
            stopFaceAnimation();
            endEyesFollowing();
        } else if (oldState == PetAnimationState::Sleep) {
            stopFaceAnimation();
            stopEyesSleepTransition();
            stopEyesHomingAnimation();
        }
        
        if (curAnimationState == PetAnimationState::None) {
            updateWindow();
        } else if (curAnimationState == PetAnimationState::Idle) {
            playFaceIdleAnimation();
            startEyesFollowing();
        } else if (curAnimationState == PetAnimationState::Sleep) {
            playEyesHomingAnimation();
            playFaceSleepTransition(); // connect playFaceSleepAnimation() & playEyesSleepTransition()
        }
        
        if (window) {
            window->update();
        }
    }
}

PetAnimationState PetController::getAnimationState() const {
    return curAnimationState;
}

void PetController::updateWindow() {
    if (window) {
        window->update();
    }
}

void PetController::loadSleepFrames() {
    sleepFrames.clear();
    for (int i = 0; i < 27; i++) {
        QString path = QString(":/resources/images/eyes/sleep/%1.png").arg(i);
        QPixmap pixmap(path);
        if (pixmap.isNull()) {
            qDebug() << "can't load sleep frame " << i << " path:" << path;
        } else {
            QPixmap scaledPixmap = pixmap.scaled(eyes_w, eyes_h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            sleepFrames.append(scaledPixmap);
        }
    }
}

void PetController::playFaceIdleAnimation() {
    if (scaleXAnimation->state() == QPropertyAnimation::Running || scaleYAnimation->state() == QPropertyAnimation::Running) {
        qDebug() << "trying to play a running animation";
        return;
    }
    if (curAnimationState == PetAnimationState::Idle) {
        scaleXAnimation->setDuration(idleAnimDuration);
        scaleXAnimation->setStartValue(1.0f);
        scaleXAnimation->setEndValue(idleMaxScaleX);
        scaleXAnimation->setEasingCurve(QEasingCurve::InOutSine);

        scaleYAnimation->setDuration(idleAnimDuration);
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

        scaleXAnimation->start();
        scaleYAnimation->start();
    }
}

void PetController::playFaceSleepAnimation() {
    if (scaleXAnimation->state() == QPropertyAnimation::Running || scaleYAnimation->state() == QPropertyAnimation::Running) {
        qDebug() << "trying to play a running animation";
        return;
    }

    scaleXAnimation->setDuration(sleepAnimDuration);
    scaleXAnimation->setStartValue(1.0f);
    scaleXAnimation->setEndValue(sleepMaxScaleX);
    scaleXAnimation->setEasingCurve(QEasingCurve::InOutSine);

    scaleYAnimation->setDuration(sleepAnimDuration);
    scaleYAnimation->setStartValue(1.0f);
    scaleYAnimation->setEndValue(sleepMaxScaleY);
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

    scaleXAnimation->start();
    scaleYAnimation->start();
}

void PetController::playFaceSleepTransition() {
    if (isPlayingFaceSleepTransition) {
        return;
    }
    if (curAnimationState == PetAnimationState::Sleep) {
        if (scaleXAnimation->state() == QPropertyAnimation::Running || scaleYAnimation->state() == QPropertyAnimation::Running) {
            qDebug() << "trying to play a running animation";
            return;
        }

        isPlayingFaceSleepTransition = true;

        scaleXAnimation->setDuration(1000);
        scaleXAnimation->setStartValue(m_curScaleX);
        scaleXAnimation->setEndValue(0.96f);
        scaleXAnimation->setEasingCurve(QEasingCurve::InOutSine);
        scaleXAnimation->setDirection(QPropertyAnimation::Forward);

        scaleYAnimation->setDuration(1000);
        scaleYAnimation->setStartValue(m_curScaleY);
        scaleYAnimation->setEndValue(1.2f);
        scaleYAnimation->setEasingCurve(QEasingCurve::InOutSine);
        scaleYAnimation->setDirection(QPropertyAnimation::Forward);

        connect(scaleXAnimation, &QPropertyAnimation::finished, this, [this]() {
            isPlayingFaceSleepTransition = false;
            stopFaceAnimation();
            playFaceSleepAnimation();
            playEyesSleepTransition();
        });

        scaleXAnimation->start();
        scaleYAnimation->start();
    }
}

void PetController::stopFaceAnimation() {
    scaleXAnimation->disconnect();
    scaleYAnimation->disconnect();
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
    }
}

void PetController::updateEyesPos() {
    updateMousePos();
    QPoint windowPos = window->pos();
    QSize windowSize = window->size();
    QPointF windowCenter = QPointF(windowPos.x() + windowSize.width() / 2, windowPos.y() + windowSize.height() / 2);
    QPointF dir = globalMousePos - windowCenter;
    float dis = std::sqrt(dir.x() * dir.x() + dir.y() * dir.y());

    if (dis < 1.0f) {
        eyesCurCenterPos = eyesCenterPos;
        return;
    }

    float nX = dir.x() / dis;
    float nY = dir.y() / dis;
    float offsetAmount = std::min(dis * sensitivity, maxOffset);
    QPointF offset = QPointF(nX * offsetAmount, nY * offsetAmount);
    eyesCurCenterPos = eyesSmoothedCenterPos + offset;

    static QPointF smoothedPos = eyesSmoothedCenterPos;
    smoothedPos += (eyesSmoothedCenterPos - smoothedPos) * 0.3;
    eyesSmoothedCenterPos = smoothedPos;

    updateWindow();
}

void PetController::startEyesFollowing() {
    if (!isEyesFollowing) {
        QPixmap o_eyesPixmap(":/resources/images/eyes/eyes_default.png");
        eyesPixmap = o_eyesPixmap.scaled(eyes_w, eyes_h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        eyesFollowTimer->start(50);
        isEyesFollowing = true;
    }
}

void PetController::endEyesFollowing() {
    if (isEyesFollowing) {
        eyesFollowTimer->stop();
        isEyesFollowing = false;
    }
}

void PetController::playEyesHomingAnimation() {
    if (isPlayingEyesHoming) {
        return;
    }

    if (eyesHomingAnimation->state() == QPropertyAnimation::Running) {
        qDebug() << "trying to play a running animation";
        return;
    }

    isPlayingEyesHoming = true;

    eyesHomingAnimation->setDuration(50);
    eyesHomingAnimation->setStartValue(eyesCurCenterPos);
    eyesHomingAnimation->setEndValue(eyesCenterPos);
    eyesHomingAnimation->setEasingCurve(QEasingCurve::InOutSine);

    eyesHomingAnimation->start();
}

void PetController::stopEyesHomingAnimation() {
    if (isPlayingEyesHoming) {
        eyesHomingAnimation->stop();
        isPlayingEyesHoming = false;
    }
}

void PetController::playEyesSleepTransition() {
    if (isPlayingEyesSleepTransition) {
        return;
    }
    if (curAnimationState == PetAnimationState::Sleep) {
        isPlayingEyesSleepTransition = true;

        sleepFrameIndex = 0;
        sleepFrameTimer->start(20);
    }
}

void PetController::stopEyesSleepTransition() {
    if (isPlayingEyesSleepTransition) {

        isPlayingEyesSleepTransition = false;

        sleepFrameTimer->stop();
        sleepFrameIndex = 0;

        QPixmap o_eyesSleepPixmap(":/resources/images/eyes/eyes_sleep.png");
        eyesPixmap = o_eyesSleepPixmap.scaled(eyes_w, eyes_h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        updateWindow();
    }
}

void PetController::updateEyesSleepFrame() {
    if (!isPlayingEyesSleepTransition) {
        return;
    }

    if (sleepFrameIndex < sleepFrames.size()) {
        eyesPixmap = sleepFrames[sleepFrameIndex];
        sleepFrameIndex++;
        updateWindow();
    } else {
        stopEyesSleepTransition();
    }
}

// propertyAnimation

void PetController::setCurScaleX(float scale) {
    if (qAbs(m_curScaleX - scale) > 0.001f) {
        m_curScaleX = scale;
        emit curScaleXChanged();
        updateWindow();
    }
}

void PetController::setCurScaleY(float scale) {
    if (qAbs(m_curScaleY - scale) > 0.001f) {
        m_curScaleY = scale;
        emit curScaleYChanged();
        updateWindow();
    }
}

void PetController::setCurEyesPos(QPointF pos) {
    if (qAbs(eyesCurCenterPos.x() - pos.x()) > 0.001f || qAbs(eyesCurCenterPos.y() - pos.y()) > 0.001f) {
        eyesCurCenterPos = pos;
        emit curEyesPosChanged();
        updateWindow();
    }
}

void PetController::setupConnection() {
    // timer
    connect(eyesFollowTimer, &QTimer::timeout, this, &PetController::updateEyesPos);
    connect(sleepFrameTimer, &QTimer::timeout, this, &PetController::updateEyesSleepFrame);

    // propertyAnimation
    connect(eyesHomingAnimation, &QPropertyAnimation::valueChanged, this, [this](const QVariant& value) {
        eyesCurCenterPos = value.toPointF();
        updateWindow();
    });

    connect(scaleXAnimation, &QPropertyAnimation::valueChanged, this, [this]() {
        updateWindow();
    });
}
