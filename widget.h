#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
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

private:
    void loadIdleAnimation();

    QVector<QPixmap> idleFrames;
    int cur_frame;
    QTimer *animTimer;
    int frame_delay;
};
#endif // WIDGET_H
