#include "MediaPushWindow.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MediaPushWindow w;
    w.show();
    return a.exec();
}
