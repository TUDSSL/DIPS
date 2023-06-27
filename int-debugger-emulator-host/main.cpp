#include "emulator.h"

#include <QApplication>

/**
 * Main function of QT Application
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Emulator w;
    w.show();
    return a.exec();
}
