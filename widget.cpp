#include "widget.h"

Widget::Widget(QWidget *parent) : QWidget(parent) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setFixedSize(250, 250);
}

Widget::~Widget() {}

void Widget::paintEvent(QPaintEvent *event)
{
    int imgW = 220;
    int imgH = 220;

    QPainter painter(this);

    QPixmap oPixmap(":/resources/images/face/face_default.png");
    QPixmap sPixmap = oPixmap.scaled(imgW, imgH, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    int x = (width() - sPixmap.width()) / 2;
    int y = (height() - sPixmap.height()) / 2;

    painter.drawPixmap(x, y, sPixmap);
}
