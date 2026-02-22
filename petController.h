#ifndef PETCONTROLLER_H
#define PETCONTROLLER_H

#include <QObject>
#include <QPropertyAnimation>
#include <QTimer>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>

class PetWindow;

class PetController : public QObject
{
    Q_OBJECT

public:
    PetController(PetWindow* parentWindow, QObject *parent = nullptr);
    ~PetController();

    QRect getImageRect() const;
    void render(QPainter* painter);

private:
    PetWindow* window;

    // face
    QPixmap facePixmap;
    int img_w;
    int img_h;
    int img_lx;
    int img_ly;

    // eyes following
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

    // animation
    QVector<QPixmap> idleFrames;
    int cur_frame;
    QTimer *animTimer;
    int frame_delay;
    void loadIdleAnimation();
};

#endif // PETCONTROLLER_H
