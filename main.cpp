#include "petWindow.h"
#include "petController.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    PetWindow w;
    
    PetController* controller = new PetController(&w, &w);
    w.setPetController(controller);
    
    w.show();
    return a.exec();
}
