#pragma once

#include <QMainWindow>
#include <QTimer>

class QCustomPlot;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);

    void setupStretchItemDemo(QCustomPlot* customPlot);

private:
    QCustomPlot* mCustomPlot;
};
