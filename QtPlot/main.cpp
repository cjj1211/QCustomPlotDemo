#include "QtPlot.h"
#include "grapph.h"
#include <QtWidgets/QApplication>
#include <mainwindow.h>
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
   /* QtPlot window;
    window.show();*/
	/*CGraph graph;
    graph.show();*/
    MainWindow w;
    w.showFullScreen(); 
    return app.exec();
}
