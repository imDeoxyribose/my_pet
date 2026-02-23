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
    Idle
};

class PetController : public QObject
{
    Q_OBJECT

public:
    PetController(PetWindow* parentWindow, QObject *parent = nullptr);
    ~PetController();

    QRect getImageRect() const;
    void render(QPainter* painter);
    void setAnimationState(PetAnimationState state);
    PetAnimationState getAnimationState() const;

private:
    PetWindow* window;

    QPixmap facePixmap;
    int img_w;
    int img_h;
    int img_lx;
    int img_ly;

    QPixmap eyesPixmap;
    QPointF eyesBaseCenterPos;
    QPointF eyesCurCenterPos;
    QPointF globalMousePos;
    QTimer* eyesTimer;

    bool enable_EyesFollowing;
    float maxOffset;
    float sensitivity;
    int eyes_w;
    int eyes_h;

    QPointF toEyesCenterAlignedPos(QPointF pos);
    void updateMousePos();
    void calculateEyesPos();

    PetAnimationState currentAnimationState;
    int cur_frame;
    QTimer *animTimer;
    int frame_delay;
    void updateAnimation();

    float maxScaleX;
    float maxScaleY;
    float currentScaleX;
    float currentScaleY;
    float scaleProgress;
    bool isScalingUp;
};

#endif // PETCONTROLLER_H
