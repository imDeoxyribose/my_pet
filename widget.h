#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QDebug>
#include <QTimer>
#include <QVector>
#include <QPropertyAnimation>
#include <QApplication>
#include <QPointF>

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:

    void loadIdleAnimation();

    QVector<QPixmap> idleFrames;
    int cur_frame;
    QTimer *animTimer;
    int frame_delay;

    // drag
    QPropertyAnimation* moveAnim;
    QPoint mouseStartPos;
    QPoint dragOffset;
    bool is_dragging;

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

    float maxOffset;
    float sensitivity;
    int eyes_w;
    int eyes_h;

    QPointF toEyesCenterAlignedPos(QPointF pos);
    void updateMousePos();
    void calculateEyesPos();
    // QPointF getOffsetFromMouse();

};
#endif // WIDGET_H
