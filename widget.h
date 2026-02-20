#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QDebug>
#include <QTimer>
#include <QVector>

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

    QPoint dragPos;
    bool is_dragging;
};
#endif // WIDGET_H
