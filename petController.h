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
    Q_PROPERTY(float currentScaleX READ currentScaleX WRITE setCurrentScaleX NOTIFY currentScaleXChanged)
    Q_PROPERTY(float currentScaleY READ currentScaleY WRITE setCurrentScaleY NOTIFY currentScaleYChanged)

public:
    PetController(PetWindow* parentWindow, QObject *parent = nullptr);
    ~PetController();

    QRect getImageRect() const;
    void render(QPainter* painter);
    void setAnimationState(PetAnimationState state);
    PetAnimationState getAnimationState() const;

    float currentScaleX() const { return m_currentScaleX; }
    float currentScaleY() const { return m_currentScaleY; }
    void setCurrentScaleX(float scale);
    void setCurrentScaleY(float scale);

signals:
    void currentScaleXChanged();
    void currentScaleYChanged();

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
    QPropertyAnimation* scaleXAnimation;
    QPropertyAnimation* scaleYAnimation;
    void startAnimation(PetAnimationState state);
    void stopAnimation(PetAnimationState state);

    float idleMaxScaleX;
    float idleMaxScaleY;
    float sleepMaxScaleX;
    float sleepMaxScaleY;
    float m_currentScaleX;
    float m_currentScaleY;
    int idleAnimDuration;
    int sleepAnimDuration;
};

#endif // PETCONTROLLER_H
