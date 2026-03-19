#pragma once
#include <qcustomplot.h>
class CGraph: public QMainWindow
{
	Q_OBJECT
public:
	CGraph(QWidget* parent = nullptr);

	QCustomPlot* customPlot;
};