#include "grapph.h"
#include "qcustomplot.h"
#include <QVBoxLayout>
#include <QWidget>  // 必须加

CGraph::CGraph(QWidget* parent)
    : QMainWindow(parent)  // 你是 QMainWindow，保持不变
{
    // ========== QMainWindow 必须设置中央控件 ==========
    QWidget* centralWidget = new QWidget(this);
    this->setCentralWidget(centralWidget);

    // 创建绘图控件
    customPlot = new QCustomPlot;
    customPlot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QCPAxis* keyAxis = customPlot->xAxis;
    QCPAxis* valueAxis = customPlot->yAxis;
    QCPBars* fossil = new QCPBars(keyAxis, valueAxis);  // 使用xAxis作为柱状图的key轴，yAxis作为value轴

    fossil->setAntialiased(false); // 为了更好的边框效果，关闭抗齿锯
    fossil->setName("Fossil fuels"); // 设置柱状图的名字，可在图例中显示
    fossil->setPen(QPen(QColor(0, 168, 140).lighter(130))); // 设置柱状图的边框颜色
    fossil->setBrush(QColor(0, 168, 140));  // 设置柱状图的画刷颜色
    QVector<double> ticks;
    QVector<QString> labels;
    ticks << 1 << 2 << 3 << 4 << 5 << 6 << 7;
    labels << "USA" << "Japan" << "Germany" << "France" << "UK" << "Italy" << "Canada";
    QSharedPointer<QCPAxisTickerText> textTicker(new QCPAxisTickerText);
    textTicker->addTicks(ticks, labels);

    keyAxis->setTicker(textTicker);        // 设置为文字轴

    keyAxis->setTickLabelRotation(60);     // 轴刻度文字旋转60度
    keyAxis->setSubTicks(false);           // 不显示子刻度
    keyAxis->setTickLength(0, 4);          // 轴内外刻度的长度分别是0,4,也就是轴内的刻度线不显示
    keyAxis->setRange(0, 8);               // 设置范围
    keyAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);

    valueAxis->setRange(0, 12.1);
    valueAxis->setPadding(35);             // 轴的内边距，可以到QCustomPlot之开始（一）看图解 
    valueAxis->setLabel("Power Consumption in\nKilowatts per Capita (2007)");
    valueAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);
    QVector<double> fossilData;
    fossilData << 0.86 * 10.5 << 0.83 * 5.5 << 0.84 * 5.5 << 0.52 * 5.8 << 0.89 * 5.2 << 0.90 * 4.2 << 0.67 * 11.2;
    fossil->setData(ticks, fossilData);
  //  // ======================
  //  // 生成 X 坐标（共用）
  //  // ======================
  //  QVector<double> x(101);
  //  for (int i = 0; i < 101; ++i) {
  //      x[i] = i / 50.0 - 1; // -1 到 1
  //  }

  //  // ======================
  //  // 第一条曲线：y = x²
  //  // ======================
  //  QVector<double> y1(101);
  //  for (int i = 0; i < 101; ++i) {
  //      y1[i] = x[i] * x[i];
  //  }

  //  // ======================
  //  // 第二条曲线：y = sin(x)
  //  // ======================
  //  QVector<double> y2(101);
  //  for (int i = 0; i < 101; ++i) {
  //      y2[i] = sin(x[i] * 3); // 正弦曲线
  //  }

  //  // ======================
  //  // 画第一条线 → graph(0)
  //  // ======================
  //  customPlot->addGraph();
  //  customPlot->graph(0)->setData(x, y1);
  //  customPlot->graph(0)->setName(QString::fromLocal8Bit("抛物线 y = x²"));
  //  customPlot->graph(0)->setPen(QPen(Qt::red)); // 红色

  //  // ======================
  //  // 画第二条线 → graph(1)
  //  // ======================
  //  customPlot->addGraph();
  //  customPlot->graph(1)->setData(x, y2);
  //  customPlot->graph(1)->setName(QString::fromLocal8Bit("正弦曲线 y = sin(3x)"));
  //  customPlot->graph(1)->setPen(QPen(Qt::blue)); // 蓝色

  //  // 坐标轴设置
  //  customPlot->xAxis->setLabel("x");
  //  customPlot->yAxis->setLabel("y");
  //  customPlot->xAxis->setRange(-1, 1);
  ////  customPlot->yAxis->setRange(-1.5, 1.5); // 范围放大一点，能看到正弦波
  ////  customPlot->legend->setVisible(true);   // 显示图例

  // // customPlot->replot(); // 必须重绘

    // ========== 布局 ==========
    QVBoxLayout* layout = new QVBoxLayout(centralWidget);
    layout->addWidget(customPlot);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
   // QLinearGradient plotGradient;
  /////*  plotGradient.setStart(0, 0);
  ////  plotGradient.setFinalStop(0, 150);*/
  // // plotGradient.setColorAt(0, QColor(80, 80, 80));
  ////  plotGradient.setColorAt(1, QColor(50, 50, 50));
  ////  customPlot->setBackground(plotGradient);      // 设置背景
  ////  customPlot->graph(0)->setChannelFillGraph(customPlot->graph(1));   // 将图0与图1围成区域
  ////  showFullScreen();
  ////  customPlot->graph(0)->setBrush(QColor(0, 150, 255, 80));
}