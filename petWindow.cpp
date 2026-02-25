#include "petWindow.h"
#include "petController.h"

PetWindow::PetWindow(QWidget *parent) : QWidget(parent), petController(nullptr) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setFixedSize(300, 300);

    dragAnim = new QPropertyAnimation(this, "pos");
    dragAnim->setDuration(200);
    dragAnim->setEasingCurve(QEasingCurve::OutQuad);

    setWindowIcon(QIcon(":/resources/images/thumbnail.png"));
}

PetWindow::~PetWindow() {}

QRect PetWindow::getWindowRect() const {
    return QRect(pos(), size());
}

void PetWindow::setPetController(PetController* controller) {
    petController = controller;
}

QRect PetWindow::getPetImageRect() const {
    if (petController) {
        return petController->getImageRect();
    }
    return QRect(0, 0, width(), height());
}

void PetWindow::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    if (petController) {
        petController->render(&painter);
    }
}

void PetWindow::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (dragAnim->state() == QPropertyAnimation::Running) {
            dragAnim->stop();
        }
        mouseStartPos = event->globalPosition().toPoint();
        dragOffset = event->globalPosition().toPoint() - frameGeometry().topLeft();
        is_dragging = true;
        event->accept();

        if (petController) {
            petController->setAnimationState(PetAnimationState::Caught);
        }
    }
}

void PetWindow::mouseMoveEvent(QMouseEvent* event) {
    if (is_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - dragOffset);
        event->accept();
    }
}

void PetWindow::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        is_dragging = false;

        int min_dis = -30;

        QPoint releasePos = pos();
        QPoint targetPos = releasePos;

        QRect petRect = getPetImageRect();

        QScreen* screen = QApplication::primaryScreen();
        QRect screenRect = screen->geometry();

        if (releasePos.x() + petRect.left() < min_dis) {
            targetPos.setX(min_dis - petRect.left());
        } else if (releasePos.x() + petRect.right() > screenRect.width() - min_dis) {
            targetPos.setX(screenRect.width() - min_dis - petRect.width() - petRect.left());
        }

        if (releasePos.y() + petRect.top() < min_dis) {
            targetPos.setY(min_dis - petRect.top());
        } else if (releasePos.y() + petRect.bottom() > screenRect.height() - min_dis) {
            targetPos.setY(screenRect.height() - min_dis - petRect.height() - petRect.top());
        }

        if (targetPos != releasePos) {
            dragAnim->setStartValue(releasePos);
            dragAnim->setEndValue(targetPos);
            dragAnim->start();
        }

        event->accept();

        if (petController) {
            petController->setAnimationState(PetAnimationState::Idle);
        }
    }
}

void PetWindow::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu(this);
    
    menu.setStyleSheet(R"(
        QMenu {
            background-color: #f5f5f5;
            border: 1px solid #d0d0d0;
            border-radius: 6px;
            padding: 3px;
            font-family: "Microsoft YaHei UI", "Segoe UI", Arial;
            font-size: 12px;
            color: #000000;
        }
        QMenu::item {
            padding: 6px 20px 6px 20px;
            border-radius: 3px;
            margin: 1px;
        }
        QMenu::item:selected {
            background-color: #e0e0e0;
            color: #000000;
        }
        QMenu::item:pressed {
            background-color: #d0d0d0;
        }
        QMenu::separator {
            height: 1px;
            background-color: #e0e0e0;
            margin: 4px 10px;
        }
    )");
    
    // Close
    QAction* closeAction = menu.addAction("退出");
    closeAction->setShortcut(QKeySequence("Alt+F4"));
    connect(closeAction, &QAction::triggered, this, []() {
        QApplication::quit();
    });
    
    menu.addSeparator();
    
    // Sleep / Wakeup
    QAction* sleepAction = nullptr;
    if (petController) {
        PetAnimationState currentState = petController->getAnimationState();
        if (currentState == PetAnimationState::Sleep) {
            sleepAction = menu.addAction("唤醒");
            sleepAction->setShortcut(QKeySequence("Space"));
            connect(sleepAction, &QAction::triggered, this, [this]() {
                petController->setAnimationState(PetAnimationState::Idle);
            });
        } else {
            sleepAction = menu.addAction("睡觉");
            sleepAction->setShortcut(QKeySequence("Space"));
            connect(sleepAction, &QAction::triggered, this, [this]() {
                petController->setAnimationState(PetAnimationState::Sleep);
            });
        }
    }
    
    menu.exec(event->globalPos());
}

