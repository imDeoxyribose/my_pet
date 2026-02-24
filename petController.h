#ifndef PETCONTROLLER_H
#define PETCONTROLLER_H

#include <QObject>
#include <QPropertyAnimation>
#include <QTimer>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QVector>
#include <QPixmap>
#include <QMap>

class PetWindow;

enum class PetAnimationState {
    None,
    Idle,
    Sleep
};

class PetController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(float curScaleX READ curScaleX WRITE setCurScaleX NOTIFY curScaleXChanged)
    Q_PROPERTY(float curScaleY READ curScaleY WRITE setCurScaleY NOTIFY curScaleYChanged)
    Q_PROPERTY(QPointF curEyesPos READ curEyesPos WRITE setCurEyesPos NOTIFY curEyesPosChanged)
    Q_PROPERTY(float Z_curScaleX READ get_Z_curScaleX WRITE setZ_curScaleX NOTIFY Z_curScaleXChanged)
    Q_PROPERTY(float Z_curScaleY READ get_Z_curScaleY WRITE setZ_curScaleY NOTIFY Z_curScaleYChanged)

public:
    PetController(PetWindow* parentWindow, QObject *parent = nullptr);
    ~PetController();

    QRect getImageRect() const;

    void render(QPainter* painter);

    void setAnimationState(PetAnimationState state);
    PetAnimationState getAnimationState() const;

    // propretyAnimation
    float curScaleX() const { return m_curScaleX; }
    float curScaleY() const { return m_curScaleY; }
    void setCurScaleX(float scale);
    void setCurScaleY(float scale);
    QPointF curEyesPos() const { return eyesCurCenterPos; }
    void setCurEyesPos(QPointF pos);
    float get_Z_curScaleX() const { return Z_curScaleX; }
    float get_Z_curScaleY() const { return Z_curScaleY; }
    void setZ_curScaleX(float scale);
    void setZ_curScaleY(float scale);

signals:
    void curScaleXChanged();
    void curScaleYChanged();
    void curEyesPosChanged();
    void Z_curScaleXChanged();
    void Z_curScaleYChanged();

private:
    PetWindow* window;
    void updateWindow();

    QPixmap facePixmap;
    int img_w;
    int img_h;
    int img_lx;
    int img_ly;

    // face idle animation
    QPropertyAnimation* scaleXAnimation;
    QPropertyAnimation* scaleYAnimation;
    float m_curScaleX;
    float m_curScaleY;
    float idleMaxScaleX;
    float idleMaxScaleY;
    int idleAnimDuration;
    void playFaceIdleAnimation();
    void stopFaceAnimation();

    // face sleep animation
    float sleepMaxScaleX;
    float sleepMaxScaleY;
    int sleepAnimDuration;
    void playFaceSleepAnimation();

    // face sleep transition animation
    bool isPlayingFaceSleepTransition;
    void playFaceSleepTransition();

    QPixmap eyesPixmap;
    int eyes_w;
    int eyes_h;
    QPointF eyesSmoothedCenterPos;
    QPointF eyesCurCenterPos;

    // eyes following
    bool isEyesFollowing;
    float maxOffset;
    float sensitivity;
    QPointF globalMousePos;
    QTimer* eyesFollowTimer;
    void updateMousePos();
    void updateEyesPos();
    QPointF toEyesCenterAlignedPos(QPointF pos);
    void startEyesFollowing();
    void endEyesFollowing();

    // eyes homing animation
    bool isPlayingEyesHoming;
    QPropertyAnimation* eyesHomingAnimation;
    QPointF eyesCenterPos;
    void playEyesHomingAnimation();
    void stopEyesHomingAnimation();

    // eyes sleep transition animation
    bool isPlayingEyesSleepTransition;
    QVector<QPixmap> sleepFrames;
    int sleepFrameIndex;
    QTimer* sleepFrameTimer;
    void loadSleepFrames();
    void updateEyesSleepFrame();
    void playEyesSleepTransition();
    void stopEyesSleepTransition();

    QPixmap Z_pixmap;
    int Z_w;
    int Z_h;
    QPointF Z_pos;

    // Z sleep transition animation
    bool isPlaying_Z_transition;
    QPropertyAnimation* Z_scaleXAnimation;
    QPropertyAnimation* Z_scaleYAnimation;
    float Z_curScaleX;
    float Z_curScaleY;
    QTimer* Z_sleepTimer;
    void play_Z_sleepTransition();
    void stop_Z_sleepTransition();

    // Z sleep vanish animation
    bool isPlaying_Z_vanishAnimation;
    QVector<QPixmap> Z_vanishFrames;
    int Z_vanishFrameIndex;
    QTimer* Z_vanishTimer;
    void load_Z_vanishFrames();
    void update_Z_vanishFrame();
    void play_Z_vanishAnimation();
    void stop_Z_vanishAnimation();

    // animation state
    PetAnimationState curAnimationState;

    // setup
    void setupConnection();
};

#endif // PETCONTROLLER_H
