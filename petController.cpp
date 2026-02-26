#include "petController.h"
#include "petWindow.h"
#include <QDebug>
#include <QtMath>
#include <QEasingCurve>

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
    float eyes_k = 0.7f;
    eyes_w = img_w * eyes_k;
    eyes_h = img_h * eyes_k / 2;
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

    // Z setup
    float Z_k = 0.4f;
    Z_w = img_w * Z_k;
    Z_h = img_h * Z_k;
    QPixmap o_Z_pixmap(":/resources/images/Z/Z.png");
    Z_pixmap = o_Z_pixmap.scaled(Z_w, Z_h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    Z_pos = QPointF(img_lx + img_w * 0.55, img_ly + img_h * 0.15);

    // Z sleep transition animation
    isPlaying_Z_transition = false;
    Z_curScaleX = 0.0f;
    Z_curScaleY = 0.0f;
    Z_scaleXAnimation = new QPropertyAnimation(this, "Z_curScaleX");
    Z_scaleXAnimation->setDuration(500);
    Z_scaleXAnimation->setEasingCurve(QEasingCurve::InOutSine);
    Z_scaleYAnimation = new QPropertyAnimation(this, "Z_curScaleY");
    Z_scaleYAnimation->setDuration(500);
    Z_scaleYAnimation->setEasingCurve(QEasingCurve::InOutSine);

    // Z sleep vanish animation
    isPlaying_Z_vanishAnimation = false;
    Z_vanishFrameIndex = 0;
    Z_vanishTimer = new QTimer(this);
    load_Z_vanishFrames();

    // Z float animation
    isPlaying_Z_floatAnimation = false;
    Z_floatY = 0.0f;
    Z_rotation = 0.0f;
    Z_floatTimer = new QTimer(this);

    // timer
    renderTimer = new QTimer(this);
    renderTimer->start(10);

    // connection setup
    setupConnection();

    loadSleepFrames();

    // initialization
    setAnimationState(PetAnimationState::Idle);
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
    } else {
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

    // render Z
    if (curAnimationState != PetAnimationState::None && !Z_pixmap.isNull()) {
        float centerX = Z_pos.x() + Z_w / 2.0f;
        float centerY = Z_pos.y() + Z_h / 2.0f;
        painter->save();
        painter->translate(centerX, centerY);
        painter->translate(0, Z_floatY);
        painter->rotate(Z_rotation);
        painter->scale(Z_curScaleX, Z_curScaleY);
        painter->translate(-centerX, -centerY);
        painter->drawPixmap(Z_pos.toPoint(), Z_pixmap);
        painter->restore();
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
            stop_Z_sleepTransition();
            play_Z_vanishAnimation();
            stop_Z_floatAnimation();
        } else if (oldState == PetAnimationState::Caught) {
            stopFaceAnimation();
            quitEyesCaught();
        }

        if (curAnimationState == PetAnimationState::None) {
            updateWindow();
        } else if (curAnimationState == PetAnimationState::Idle) {
            playFaceIdleAnimation();
            startEyesFollowing();
        } else if (curAnimationState == PetAnimationState::Sleep) {
            playEyesHomingAnimation();
            playFaceSleepTransition(); // connect playFaceSleepAnimation()
            play_Z_sleepTransition();
            delay(1000, PetAnimationState::Sleep, [this]() {
                playEyesSleepTransition();
            });
        } else if (curAnimationState == PetAnimationState::Caught) {
            playEnterFaceCaughtTransition();
            enterEyesCaught();
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
        if (qAbs(m_curScaleX - 1.0f) > 0.001f || qAbs(m_curScaleY - 1.0f) > 0.001f) {
            scaleXAnimation->setDuration(200);
            scaleXAnimation->setStartValue(m_curScaleX);
            scaleXAnimation->setEndValue(1.0f);
            scaleXAnimation->setEasingCurve(QEasingCurve::InOutSine);
            scaleXAnimation->setDirection(QPropertyAnimation::Forward);

            scaleYAnimation->setDuration(200);
            scaleYAnimation->setStartValue(m_curScaleY);
            scaleYAnimation->setEndValue(1.0f);
            scaleYAnimation->setEasingCurve(QEasingCurve::InOutSine);
            scaleYAnimation->setDirection(QPropertyAnimation::Forward);

            scaleXAnimation->start();
            scaleYAnimation->start();
        }

        delay(500, PetAnimationState::Idle, [this]() {
            scaleXAnimation->setDuration(idleAnimDuration);
            scaleXAnimation->setStartValue(1.0f);
            scaleXAnimation->setEndValue(idleMaxScaleX);
            scaleXAnimation->setEasingCurve(QEasingCurve::InOutSine);

            scaleYAnimation->setDuration(idleAnimDuration);
            scaleYAnimation->setStartValue(1.0f);
            scaleYAnimation->setEndValue(idleMaxScaleY);
            scaleYAnimation->setEasingCurve(QEasingCurve::InOutSine);

            makeAnimLoop(scaleXAnimation);
            makeAnimLoop(scaleYAnimation);

            scaleXAnimation->start();
            scaleYAnimation->start();
        });
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

    makeAnimLoop(scaleXAnimation);
    makeAnimLoop(scaleYAnimation);

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

        scaleXAnimation->setDuration(500);
        scaleXAnimation->setStartValue(m_curScaleX);
        scaleXAnimation->setEndValue(0.96f);
        scaleXAnimation->setEasingCurve(QEasingCurve::InOutSine);
        scaleXAnimation->setDirection(QPropertyAnimation::Forward);

        scaleYAnimation->setDuration(500);
        scaleYAnimation->setStartValue(m_curScaleY);
        scaleYAnimation->setEndValue(1.2f);
        scaleYAnimation->setEasingCurve(QEasingCurve::InOutSine);
        scaleYAnimation->setDirection(QPropertyAnimation::Forward);

        connect(scaleXAnimation, &QPropertyAnimation::finished, this, [this]() {
            stopFaceAnimation();
            scaleXAnimation->setStartValue(m_curScaleX);
            scaleXAnimation->setEndValue(1.0f);
            scaleYAnimation->setStartValue(m_curScaleY);
            scaleYAnimation->setEndValue(1.0f);

            connect(scaleXAnimation, &QPropertyAnimation::finished, this, [this]() {
                isPlayingFaceSleepTransition = false;
                stopFaceAnimation();

                if (curAnimationState == PetAnimationState::Sleep) {
                    playFaceSleepAnimation();
                    //playEyesSleepTransition();
                }
            });

            scaleXAnimation->start();
            scaleYAnimation->start();
        });

        scaleXAnimation->start();
        scaleYAnimation->start();
    }
}

void PetController::playEnterFaceCaughtTransition() {
    if (scaleXAnimation->state() == QPropertyAnimation::Running || scaleYAnimation->state() == QPropertyAnimation::Running) {
        qDebug() << "trying to play a running animation";
        return;
    }
    if (curAnimationState == PetAnimationState::Caught) {
        scaleXAnimation->setDuration(200);
        scaleXAnimation->setStartValue(m_curScaleX);
        scaleXAnimation->setEndValue(1.05f);
        scaleXAnimation->setEasingCurve(QEasingCurve::InOutSine);
        scaleXAnimation->setDirection(QPropertyAnimation::Forward);

        scaleYAnimation->setDuration(200);
        scaleYAnimation->setStartValue(m_curScaleY);
        scaleYAnimation->setEndValue(1.05f);
        scaleYAnimation->setEasingCurve(QEasingCurve::InOutSine);
        scaleYAnimation->setDirection(QPropertyAnimation::Forward);

        scaleXAnimation->start();
        scaleYAnimation->start();
    }
}

void PetController::stopFaceAnimation() {
    isPlayingFaceSleepTransition = false;
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

    //updateWindow();
}

void PetController::startEyesFollowing() {
    if (!isEyesFollowing) {
        QPixmap o_eyesPixmap(":/resources/images/eyes/eyes_default.png");
        eyesPixmap = o_eyesPixmap.scaled(eyes_w, eyes_h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        eyesFollowTimer->start(20);
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

    if (curAnimationState == PetAnimationState::Sleep) {
        isPlayingEyesHoming = true;

        eyesHomingAnimation->setDuration(1000);
        eyesHomingAnimation->setStartValue(eyesCurCenterPos);
        eyesHomingAnimation->setEndValue(eyesCenterPos);
        eyesHomingAnimation->setEasingCurve(QEasingCurve::InOutSine);

        eyesHomingAnimation->start();
    }
}

void PetController::stopEyesHomingAnimation() {
    if (isPlayingEyesHoming) {
        eyesHomingAnimation->stop();
        isPlayingEyesHoming = false;
    }
}

void PetController::playEyesSleepTransition() {
    if (isPlayingEyesSleepTransition || sleepFrames.isEmpty()) {
        return;
    }
    if (curAnimationState == PetAnimationState::Sleep) {
        isPlayingEyesSleepTransition = true;

        sleepFrameIndex = 0;
        sleepFrameTimer->start(10);
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

void PetController::enterEyesCaught() {
    if (curAnimationState == PetAnimationState::Caught) {
        QPixmap o_eyesCaughtPixmap(":/resources/images/eyes/eyes_squeeze.png");
        eyesPixmap = o_eyesCaughtPixmap.scaled(eyes_w, eyes_h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        updateWindow();
    }
}

void PetController::quitEyesCaught() {
    if (curAnimationState != PetAnimationState::Caught) {
        QPixmap o_eyesPixmap(":/resources/images/eyes/eyes_default.png");
        eyesPixmap = o_eyesPixmap.scaled(eyes_w, eyes_h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
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
        //updateWindow();
    } else {
        stopEyesSleepTransition();
    }
}

void PetController::load_Z_vanishFrames() {
    Z_vanishFrames.clear();
    for (int i = 0; i < 17; i++) {
        QString path = QString(":/resources/images/Z/vanish/%1.png").arg(i);
        QPixmap pixmap(path);
        if (pixmap.isNull()) {
            qDebug() << "can't load sleep frame " << i << " path:" << path;
        } else {
            QPixmap scaledPixmap = pixmap.scaled(Z_w, Z_h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            Z_vanishFrames.append(scaledPixmap);
        }
    }
}

void PetController::play_Z_sleepTransition() {
    if (isPlaying_Z_transition) {
        return;
    }

    if (Z_scaleXAnimation->state() == QPropertyAnimation::Running || Z_scaleYAnimation->state() == QPropertyAnimation::Running) {
        qDebug() << "trying to play a running animation";
        return;
    }

    if (Z_pixmap.isNull()) {
        QPixmap o_Z_pixmap(":/resources/images/Z/Z.png");
        Z_pixmap = o_Z_pixmap.scaled(Z_w, Z_h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    if (curAnimationState == PetAnimationState::Sleep) {
        isPlaying_Z_transition = true;

        delay(2000, isPlaying_Z_transition, [this]() {

            Z_scaleXAnimation->setDuration(1000);
            Z_scaleXAnimation->setStartValue(Z_curScaleX);
            Z_scaleXAnimation->setEndValue(1.0f);
            Z_scaleXAnimation->setDirection(QPropertyAnimation::Forward);

            Z_scaleYAnimation->setDuration(1000);
            Z_scaleYAnimation->setStartValue(Z_curScaleY);
            Z_scaleYAnimation->setEndValue(1.0f);
            Z_scaleYAnimation->setDirection(QPropertyAnimation::Forward);

            connect(Z_scaleXAnimation, &QPropertyAnimation::finished, this, &PetController::play_Z_floatAnimation, Qt::SingleShotConnection);

            Z_scaleXAnimation->start();
            Z_scaleYAnimation->start();
        });
    }
}

void PetController::stop_Z_sleepTransition() {
    if (isPlaying_Z_transition) {
        isPlaying_Z_transition = false;
        Z_scaleXAnimation->stop();
        Z_scaleYAnimation->stop();
    }
}

void PetController::play_Z_vanishAnimation() {
    if (isPlaying_Z_vanishAnimation || Z_vanishFrames.isEmpty()) {
        return;
    }

    isPlaying_Z_vanishAnimation = true;

    Z_vanishFrameIndex = 0;
    Z_vanishTimer->start(10);
}

void PetController::stop_Z_vanishAnimation() {
    if (isPlaying_Z_vanishAnimation) {

        isPlaying_Z_vanishAnimation = false;

        Z_vanishTimer->stop();
        Z_vanishFrameIndex = 0;

        Z_curScaleX = 0.0f;
        Z_curScaleY = 0.0f;

        Z_pixmap = QPixmap();
        updateWindow();
    }
}

void PetController::update_Z_vanishFrame() {
    if (!isPlaying_Z_vanishAnimation || Z_vanishFrames.isEmpty()) {
        return;
    }

    if (Z_vanishFrameIndex < Z_vanishFrames.size()) {
        Z_pixmap = Z_vanishFrames[Z_vanishFrameIndex];
        Z_vanishFrameIndex++;
        //updateWindow();
    } else {
        stop_Z_vanishAnimation();
    }
}

void PetController::play_Z_floatAnimation() {
    if (isPlaying_Z_floatAnimation) {
        return;
    }
    if (curAnimationState == PetAnimationState::Sleep) {
        isPlaying_Z_floatAnimation = true;
        Z_floatTimer->start(10);
    }
}

void PetController::stop_Z_floatAnimation() {
    if (isPlaying_Z_floatAnimation) {
        isPlaying_Z_floatAnimation = false;
        Z_floatTimer->stop();
    }
}

void PetController::update_Z_floatAnimation() {
    if (!isPlaying_Z_floatAnimation) {
        return;
    }
    if (curAnimationState == PetAnimationState::Sleep) {
        static float floatSpeed = 1.0f;
        static float rotationSpeed = 0.5f;
        static float floatAmplitude = 5.0f;
        static float rotationAmplitude = 10.0f;
        static float time = 0.0f;

        time += 0.02f;

        float randomFactor1 = 0.5f + 0.5f * qSin(time * 0.3f);
        float randomFactor2 = 0.5f + 0.5f * qSin(time * 0.7f);

        Z_floatY = floatAmplitude * qSin(time * floatSpeed) * randomFactor1;
        Z_rotation = rotationAmplitude * qSin(time * rotationSpeed) * randomFactor2;
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

void PetController::setZ_curScaleX(float scale) {
    if (qAbs(Z_curScaleX - scale) > 0.001f) {
        Z_curScaleX = scale;
        emit Z_curScaleXChanged();
        updateWindow();
    }
}

void PetController::setZ_curScaleY(float scale) {
    if (qAbs(Z_curScaleY - scale) > 0.001f) {
        Z_curScaleY = scale;
        emit Z_curScaleYChanged();
        updateWindow();
    }
}

void PetController::set_Z_floatY(float value) {
    if (qAbs(Z_floatY - value) > 0.001f) {
        Z_floatY = value;
        emit Z_floatYChanged();
        updateWindow();
    }
}

void PetController::set_Z_rotation(float value) {
    if (qAbs(Z_rotation - value) > 0.001f) {
        Z_rotation = value;
        emit Z_rotationChanged();
        updateWindow();
    }
}

void PetController::setupConnection() {
    // timer
    connect(renderTimer, &QTimer::timeout, this, &PetController::updateWindow);
    connect(eyesFollowTimer, &QTimer::timeout, this, &PetController::updateEyesPos);
    connect(sleepFrameTimer, &QTimer::timeout, this, &PetController::updateEyesSleepFrame);
    connect(Z_vanishTimer, &QTimer::timeout, this, &PetController::update_Z_vanishFrame);
    connect(Z_floatTimer, &QTimer::timeout, this, &PetController::update_Z_floatAnimation);
}

// utility

template<typename Func>
void PetController::delay(int t, bool& flag, Func func) {
    QTimer::singleShot(t, this, [this, &flag, func]() {
        if (flag) {
            func();
        }
    });
}

template<typename Func>
void PetController::delay(int t, PetAnimationState state, Func func) {
    QTimer::singleShot(t, this, [this, state, func]() {
        if (curAnimationState == state) {
            func();
        }
    });
}

void PetController::makeAnimLoop(QPropertyAnimation* anim) {
    connect(anim, &QPropertyAnimation::finished, this, [this, anim]() {
        if (anim->direction() == QPropertyAnimation::Forward) {
            anim->setDirection(QPropertyAnimation::Backward);
        } else {
            anim->setDirection(QPropertyAnimation::Forward);
        }
        anim->start();
    });
}
