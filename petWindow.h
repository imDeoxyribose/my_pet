#ifndef PETWINDOW_H
#define PETWINDOW_H

#include <QWidget>
#include <QMouseEvent>
#include <QDebug>
#include <QPropertyAnimation>
#include <QApplication>
#include <QRect>
#include <QPaintEvent>
#include <QPainter>
#include <QMenu>
#include <QContextMenuEvent>
#include <QKeySequence>

class PetController;

class PetWindow : public QWidget
{
    Q_OBJECT

public:
    PetWindow(QWidget *parent = nullptr);
    ~PetWindow();

    QRect getWindowRect() const;
    void setPetController(PetController* controller);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    PetController* petController;

    // drag
    QPropertyAnimation* dragAnim;
    QPoint mouseStartPos;
    QPoint dragOffset;
    bool is_dragging;

    QRect getPetImageRect() const;
};
#endif // PETWINDOW_H
