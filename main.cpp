#include "QtPlot.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QtPlot window;
    window.show();
    return app.exec();
}
