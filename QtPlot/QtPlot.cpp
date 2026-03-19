#include "QtPlot.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QScrollArea>
#include <QGridLayout>
#include <QTime>
#include <QDateTime>
#include <QRandomGenerator>
#include <QShowEvent>
#include <QScreen>
#include <QApplication>
#include <QLineEdit>
#include <QColorDialog>
#include <QPainter>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDebug>
#include <cmath>

/**
 * @brief 构造函数
 * @param parent 父窗口
 *
 * 初始化所有成员变量，设置UI，创建定时器
 */
QtPlot::QtPlot(QWidget* parent)
    : QMainWindow(parent)
    , m_plotContainer(nullptr)
    , m_mainLayout(nullptr)
    , m_plotLayout(nullptr)
    , m_channelControlLayout(nullptr)
    , m_updateTimer(nullptr)
    , m_currentTimeTimer(nullptr)
    , m_runTimeTimer(nullptr)
    , m_runTimeLabel(nullptr)
    , m_currentTimeLabel(nullptr)
    , m_dataRateLabel(nullptr)
    , m_currentTime(0)
    , m_maxDataPoints(500)
    , m_updateInterval(50)
    , m_isUpdating(false)
    , m_elapsedSeconds(0.0)
    , m_isFullScreen(false)
    , m_dataUpdateCount(0)
    , m_zoomChannelIndex(-1)
    , m_exitZoomBtn(nullptr)
    , m_randomGenerator(static_cast<quint32>(QDateTime::currentMSecsSinceEpoch()))
{
    setupUI();
    setupPlot();
    setupConnections();

    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(m_updateInterval);
    connect(m_updateTimer, &QTimer::timeout, this, &QtPlot::updateData);

    m_currentTimeTimer = new QTimer(this);
    m_currentTimeTimer->setInterval(100);
    connect(m_currentTimeTimer, &QTimer::timeout, this, &QtPlot::updateTimeDisplay);
    m_currentTimeTimer->start();

    m_runTimeTimer = new QTimer(this);
    m_runTimeTimer->setInterval(100);
    connect(m_runTimeTimer, &QTimer::timeout, this, [this]() {
        if (m_isUpdating) {
            m_elapsedSeconds += 0.1;
            updateTimeDisplay();
        }
        });
}

/**
 * @brief 析构函数
 *
 * 停止所有定时器，清理资源
 */
QtPlot::~QtPlot()
{
    if (m_updateTimer && m_updateTimer->isActive()) {
        m_updateTimer->stop();
    }
    if (m_currentTimeTimer && m_currentTimeTimer->isActive()) {
        m_currentTimeTimer->stop();
    }
    if (m_runTimeTimer && m_runTimeTimer->isActive()) {
        m_runTimeTimer->stop();
    }
}

/**
 * @brief 窗口显示事件
 * @param event 显示事件
 *
 * 窗口显示时自动进入全屏模式
 */
void QtPlot::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);

    if (!m_isFullScreen) {
        showFullScreen();
        m_isFullScreen = true;
    }
}

/**
 * @brief 设置用户界面
 *
 * 创建主窗口、时间栏、绘图区域、控制面板等
 */
void QtPlot::setupUI()
{
    setWindowTitle(QString::fromLocal8Bit("多通道实时数据监控系统"));

    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    m_mainLayout = new QVBoxLayout(centralWidget);
    m_mainLayout->setSpacing(8);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);

    // 顶部时间栏
    QWidget* timeBar = new QWidget(this);
    timeBar->setFixedHeight(60);
    timeBar->setStyleSheet("background-color: #2c3e50; border-bottom: 1px solid #34495e;");

    QHBoxLayout* timeLayout = new QHBoxLayout(timeBar);
    timeLayout->setSpacing(30);
    timeLayout->setContentsMargins(20, 5, 20, 5);

    m_runTimeLabel = new QLabel(QString::fromLocal8Bit("运行时间: 00:00:00.000"), timeBar);
    m_runTimeLabel->setStyleSheet("color: #ecf0f1; font-size: 18px; font-weight: bold;");

    m_dataRateLabel = new QLabel(QString::fromLocal8Bit("数据速率: 0 点/秒"), timeBar);
    m_dataRateLabel->setStyleSheet("color: #3498db; font-size: 18px; font-weight: bold;");

    m_currentTimeLabel = new QLabel(timeBar);
    m_currentTimeLabel->setStyleSheet("color: #ecf0f1; font-size: 18px;");

    // 退出放大按钮（初始隐藏）
    m_exitZoomBtn = new QPushButton(QString::fromLocal8Bit("退出放大模式"), timeBar);
    m_exitZoomBtn->setFixedHeight(35);
    m_exitZoomBtn->setVisible(false);
    m_exitZoomBtn->setStyleSheet(
        "QPushButton { "
        "font-size: 14px; "
        "font-weight: bold; "
        "padding: 6px 12px; "
        "background-color: #e74c3c; "
        "color: white; "
        "border: none; "
        "border-radius: 4px; "
        "}"
        "QPushButton:hover { "
        "background-color: #c0392b; "
        "}"
    );
    m_exitZoomBtn->setToolTip(QString::fromLocal8Bit("退出放大模式，显示所有通道"));
    connect(m_exitZoomBtn, &QPushButton::clicked, this, &QtPlot::exitZoomMode);

    timeLayout->addWidget(m_runTimeLabel);
    timeLayout->addWidget(m_dataRateLabel);
    timeLayout->addStretch();
    timeLayout->addWidget(m_exitZoomBtn);
    timeLayout->addWidget(m_currentTimeLabel);

    m_mainLayout->addWidget(timeBar);

    // 主内容区域
    QWidget* contentWidget = new QWidget(this);
    QHBoxLayout* contentLayout = new QHBoxLayout(contentWidget);
    contentLayout->setSpacing(10);
    contentLayout->setContentsMargins(5, 5, 5, 5);

    // 左侧绘图区域
    QScrollArea* plotScrollArea = new QScrollArea(this);
    plotScrollArea->setWidgetResizable(true);
    plotScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    plotScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    plotScrollArea->setFrameShape(QFrame::NoFrame);

    m_plotContainer = new QWidget(plotScrollArea);
    m_plotLayout = new QVBoxLayout(m_plotContainer);
    m_plotLayout->setSpacing(5);
    m_plotLayout->setContentsMargins(5, 5, 5, 5);
    m_plotLayout->setAlignment(Qt::AlignTop);

    plotScrollArea->setWidget(m_plotContainer);
    contentLayout->addWidget(plotScrollArea, 4);

    // 右侧控制面板
    QWidget* controlPanel = new QWidget(this);
    controlPanel->setMaximumWidth(550);
    controlPanel->setMinimumWidth(470);
    QVBoxLayout* controlLayout = new QVBoxLayout(controlPanel);
    controlLayout->setSpacing(12);
    controlLayout->setContentsMargins(8, 8, 8, 8);

    // 全屏控制按钮
    QPushButton* fullScreenBtn = new QPushButton(QString::fromLocal8Bit("切换全屏"), this);
    fullScreenBtn->setFixedHeight(40);
    fullScreenBtn->setStyleSheet(
        "QPushButton { "
        "font-size: 14px; "
        "font-weight: bold; "
        "padding: 8px; "
        "}"
    );
    fullScreenBtn->setToolTip(QString::fromLocal8Bit("切换全屏/窗口模式"));

    // 数据更新控制组
    QGroupBox* updateGroup = new QGroupBox(QString::fromLocal8Bit("数据更新控制"), this);
    updateGroup->setStyleSheet(
        "QGroupBox { "
        "font-size: 13px; "
        "font-weight: bold; "
        "}"
    );
    QVBoxLayout* updateLayout = new QVBoxLayout(updateGroup);
    updateLayout->setSpacing(8);

    QHBoxLayout* intervalLayout = new QHBoxLayout();
    QLabel* intervalLabel = new QLabel(QString::fromLocal8Bit("更新间隔(ms):"), this);
    intervalLabel->setStyleSheet("font-size: 13px;");

    QSpinBox* intervalSpin = new QSpinBox(this);
    intervalSpin->setFixedHeight(30);
    intervalSpin->setStyleSheet(
        "QSpinBox { "
        "font-size: 13px; "
        "padding: 4px; "
        "}"
    );
    intervalSpin->setRange(1, 1000);
    intervalSpin->setValue(m_updateInterval);
    intervalSpin->setSingleStep(10);
    intervalSpin->setSuffix(" ms");

    intervalLayout->addWidget(intervalLabel);
    intervalLayout->addWidget(intervalSpin);

    QPushButton* startStopBtn = new QPushButton(QString::fromLocal8Bit("开始更新"), this);
    startStopBtn->setFixedHeight(35);
    startStopBtn->setCheckable(true);
    startStopBtn->setStyleSheet(
        "QPushButton { "
        "font-size: 13px; "
        "font-weight: bold; "
        "padding: 6px; "
        "}"
    );
    startStopBtn->setToolTip(QString::fromLocal8Bit("开始/停止数据更新"));

    updateLayout->addLayout(intervalLayout);
    updateLayout->addWidget(startStopBtn);

    // 通道数量控制组
    QGroupBox* countGroup = new QGroupBox(QString::fromLocal8Bit("通道数量控制"), this);
    countGroup->setStyleSheet(
        "QGroupBox { "
        "font-size: 13px; "
        "font-weight: bold; "
        "}"
    );
    QVBoxLayout* countLayout = new QVBoxLayout(countGroup);
    countLayout->setSpacing(8);

    QSpinBox* channelCountSpin = new QSpinBox(this);
    channelCountSpin->setFixedHeight(30);
    channelCountSpin->setStyleSheet(
        "QSpinBox { "
        "font-size: 13px; "
        "padding: 4px; "
        "}"
    );
    channelCountSpin->setRange(0, 20);
    channelCountSpin->setValue(5);
    channelCountSpin->setToolTip(QString::fromLocal8Bit("设置通道数量 (0-20)"));

    QPushButton* setCountBtn = new QPushButton(QString::fromLocal8Bit("设置通道数量"), this);
    setCountBtn->setFixedHeight(35);
    setCountBtn->setStyleSheet(
        "QPushButton { "
        "font-size: 13px; "
        "padding: 6px; "
        "}"
    );

    QPushButton* addChannelBtn = new QPushButton(QString::fromLocal8Bit("添加单个通道"), this);
    addChannelBtn->setFixedHeight(35);
    addChannelBtn->setStyleSheet(
        "QPushButton { "
        "font-size: 13px; "
        "padding: 6px; "
        "}"
    );

    QPushButton* clearAllBtn = new QPushButton(QString::fromLocal8Bit("清空所有通道"), this);
    clearAllBtn->setFixedHeight(35);
    clearAllBtn->setStyleSheet(
        "QPushButton { "
        "font-size: 13px; "
        "padding: 6px; "
        "}"
    );

    countLayout->addWidget(new QLabel(QString::fromLocal8Bit("通道数量:")));
    countLayout->addWidget(channelCountSpin);
    countLayout->addWidget(setCountBtn);
    countLayout->addWidget(addChannelBtn);
    countLayout->addWidget(clearAllBtn);

    // 通道控制组
    QGroupBox* channelGroup = new QGroupBox(QString::fromLocal8Bit("通道控制"), this);
    channelGroup->setStyleSheet(
        "QGroupBox { "
        "font-size: 13px; "
        "font-weight: bold; "
        "}"
    );

    QVBoxLayout* channelGroupLayout = new QVBoxLayout(channelGroup);
    channelGroupLayout->setSpacing(8);

    QScrollArea* channelScrollArea = new QScrollArea(this);
    channelScrollArea->setWidgetResizable(true);
    channelScrollArea->setMinimumHeight(600);
    channelScrollArea->setFrameShape(QFrame::NoFrame);

    QWidget* channelContainer = new QWidget(this);
    m_channelControlLayout = new QVBoxLayout(channelContainer);
    m_channelControlLayout->setSpacing(8);
    m_channelControlLayout->setContentsMargins(5, 5, 5, 5);
    m_channelControlLayout->setAlignment(Qt::AlignTop);

    channelScrollArea->setWidget(channelContainer);
    channelGroupLayout->addWidget(channelScrollArea);

    // 添加到控制面板
    controlLayout->addWidget(fullScreenBtn);
    controlLayout->addWidget(updateGroup);
    controlLayout->addWidget(countGroup);
    controlLayout->addWidget(channelGroup);
    controlLayout->addStretch();

    contentLayout->addWidget(controlPanel, 1);
    m_mainLayout->addWidget(contentWidget, 1);

    // 连接信号
    connect(fullScreenBtn, &QPushButton::clicked, this, &QtPlot::toggleFullScreen);
    connect(addChannelBtn, &QPushButton::clicked, this, &QtPlot::addChannel);
    connect(clearAllBtn, &QPushButton::clicked, this, &QtPlot::clearAllChannels);
    connect(setCountBtn, &QPushButton::clicked, this, [this, channelCountSpin]() {
        setChannelCount(channelCountSpin->value());
        });

    connect(startStopBtn, &QPushButton::toggled, this, [this, startStopBtn](bool checked) {
        toggleDataUpdate(checked);
        startStopBtn->setText(checked ? QString::fromLocal8Bit("停止更新") : QString::fromLocal8Bit("开始更新"));
        });

    connect(intervalSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &QtPlot::setUpdateInterval);
}

/**
 * @brief 设置绘图区域
 *
 * 初始化5个通道
 */
void QtPlot::setupPlot()
{
    for (int i = 0; i < 5; ++i) {
        addChannel();
    }

    adjustPlotLayout();
}

/**
 * @brief 设置信号槽连接
 *
 * 可在此添加额外的信号槽连接
 */
void QtPlot::setupConnections()
{
    // 可在此添加额外的信号槽连接
}

/**
 * @brief 添加通道
 *
 * 创建新的通道数据、绘图控件和控制控件
 */
void QtPlot::addChannel()
{
    int channelIndex = m_channels.size();

    // 创建通道默认波形配置
    WaveformConfig defaultWaveform;
    defaultWaveform.type = static_cast<WaveformType>(channelIndex % 5);
    defaultWaveform.frequency = 0.5 + channelIndex * 0.1;
    defaultWaveform.amplitude = 1.0;
    defaultWaveform.offset = 0.0;
    defaultWaveform.phase = 0.0;
    defaultWaveform.dutyCycle = 0.5;

    ChannelData channel;
    channel.id = channelIndex;
    channel.name = QString::fromLocal8Bit("通道 %1").arg(channelIndex + 1);
    channel.color = getChannelColor(channelIndex);
    channel.visible = true;
    channel.maxDataPoints = m_maxDataPoints;
    channel.needClear = false;
    channel.zoomMode = false;
    channel.peakToPeak = 0.0;
    channel.minValue = 0.0;
    channel.maxValue = 0.0;
    channel.waveform = defaultWaveform;

    m_channels.append(channel);

    createChannelPlot(channelIndex);

    QGroupBox* channelBox = new QGroupBox(this);
    channelBox->setCheckable(true);
    channelBox->setChecked(true);

    channelBox->setFixedHeight(250);
    channelBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    updateChannelBoxStyle(channelBox, channel.color, channel.name);

    QGridLayout* gridLayout = new QGridLayout(channelBox);
    gridLayout->setSpacing(6);
    gridLayout->setContentsMargins(10, 12, 10, 10);

    QCheckBox* visibleCheck = new QCheckBox(channelBox);
    visibleCheck->setChecked(true);
    visibleCheck->setFixedSize(20, 20);
    visibleCheck->setStyleSheet("QCheckBox { font-size: 12px; }");
    visibleCheck->setToolTip(QString::fromLocal8Bit("显示/隐藏此通道"));
    visibleCheck->setObjectName("visibleCheck");

    QLabel* nameLabel = new QLabel(QString::fromLocal8Bit("名称:"), channelBox);
    nameLabel->setStyleSheet("font-size: 12px; font-weight: bold;");
    nameLabel->setObjectName("nameLabel");

    QLineEdit* nameEdit = new QLineEdit(channel.name, channelBox);
    nameEdit->setMinimumWidth(100);
    nameEdit->setFixedHeight(28);
    nameEdit->setStyleSheet(
        "QLineEdit { "
        "border: 1px solid #cccccc; "
        "border-radius: 3px; "
        "padding: 4px; "
        "font-size: 12px; "
        "}"
    );
    nameEdit->setObjectName("nameEdit");

    QLabel* colorLabel = new QLabel(QString::fromLocal8Bit("颜色:"), channelBox);
    colorLabel->setStyleSheet("font-size: 12px; font-weight: bold;");
    colorLabel->setObjectName("colorLabel");

    QToolButton* colorBtn = new QToolButton(channelBox);
    colorBtn->setFixedSize(30, 30);
    colorBtn->setToolTip(QString::fromLocal8Bit("点击选择颜色"));
    updateColorButtonIcon(colorBtn, channel.color);
    colorBtn->setStyleSheet(
        "QToolButton { "
        "border: 2px solid #666666; "
        "border-radius: 5px; "
        "background-color: transparent; "
        "padding: 3px; "
        "}"
        "QToolButton:hover { "
        "border: 3px solid #000000; "
        "background-color: #f0f0f0; "
        "}"
    );
    colorBtn->setObjectName("colorBtn");

    // 放大按钮
    QPushButton* zoomBtn = new QPushButton(QString::fromLocal8Bit("放大"), channelBox);
    zoomBtn->setFixedHeight(28);
    zoomBtn->setFixedWidth(60);
    zoomBtn->setStyleSheet(
        "QPushButton { "
        "font-size: 12px; "
        "padding: 4px; "
        "background-color: #3498db; "
        "color: white; "
        "border: none; "
        "border-radius: 3px; "
        "}"
        "QPushButton:hover { "
        "background-color: #2980b9; "
        "}"
    );
    zoomBtn->setToolTip(QString::fromLocal8Bit("点击放大此通道，隐藏其他通道"));
    zoomBtn->setObjectName("zoomBtn");

    QLabel* dataLabel = new QLabel(QString::fromLocal8Bit("点数:"), channelBox);
    dataLabel->setStyleSheet("font-size: 12px; font-weight: bold;");
    dataLabel->setObjectName("dataLabel");

    QLabel* dataCountLabel = new QLabel(QString::fromLocal8Bit("0"), channelBox);
    dataCountLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    dataCountLabel->setStyleSheet(
        "QLabel { "
        "font-size: 12px; "
        "font-weight: bold; "
        "color: #333333; "
        "}"
    );
    dataCountLabel->setObjectName("dataCountLabel");

    QLabel* peakLabel = new QLabel(QString::fromLocal8Bit("峰峰值:"), channelBox);
    peakLabel->setStyleSheet("font-size: 12px; font-weight: bold;");
    peakLabel->setObjectName("peakLabel");

    QLabel* peakValueLabel = new QLabel(QString::fromLocal8Bit("0.000"), channelBox);
    peakValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    peakValueLabel->setStyleSheet(
        "QLabel { "
        "font-size: 12px; "
        "font-weight: bold; "
        "color: #e74c3c; "
        "}"
    );
    peakValueLabel->setObjectName("peakValueLabel");

    QPushButton* removeBtn = new QPushButton(QString::fromLocal8Bit("删除"), channelBox);
    removeBtn->setFixedHeight(28);
    removeBtn->setFixedWidth(60);
    removeBtn->setStyleSheet(
        "QPushButton { "
        "font-size: 12px; "
        "padding: 4px; "
        "}"
    );
    removeBtn->setToolTip(QString::fromLocal8Bit("删除此通道"));
    removeBtn->setObjectName("removeBtn");

    // 第1行
    gridLayout->addWidget(visibleCheck, 0, 0, 1, 1);
    gridLayout->addWidget(nameLabel, 0, 1, 1, 1);
    gridLayout->addWidget(nameEdit, 0, 2, 1, 2);
    gridLayout->addWidget(colorLabel, 0, 4, 1, 1);
    gridLayout->addWidget(colorBtn, 0, 5, 1, 1);
    gridLayout->addWidget(zoomBtn, 0, 6, 1, 1);

    // 第2行
    gridLayout->addWidget(dataLabel, 1, 1, 1, 1);
    gridLayout->addWidget(dataCountLabel, 1, 2, 1, 1);
    gridLayout->addWidget(peakLabel, 1, 3, 1, 1);
    gridLayout->addWidget(peakValueLabel, 1, 4, 1, 1);
    gridLayout->addWidget(removeBtn, 1, 6, 1, 1);

    // 第3行：波形配置
    createWaveformConfigWidget(channelBox, channelIndex);

    // 设置列宽比例
    for (int i = 0; i < 7; ++i) {
        gridLayout->setColumnStretch(i, 1);
    }

    m_channelWidgets.append(channelBox);
    m_channelControlLayout->insertWidget(m_channelControlLayout->count(), channelBox);

    // 连接信号
    connect(channelBox, &QGroupBox::toggled, this, [this, channelIndex](bool checked) {
        m_channels[channelIndex].visible = checked;
        if (channelIndex < m_plots.size()) {
            m_plots[channelIndex]->setVisible(checked);
        }
        adjustPlotLayout();
        });

    connect(visibleCheck, &QCheckBox::toggled, this, [this, channelIndex](bool checked) {
        m_channels[channelIndex].visible = checked;
        if (channelIndex < m_plots.size()) {
            m_plots[channelIndex]->setVisible(checked);
        }
        adjustPlotLayout();
        });

    connect(nameEdit, &QLineEdit::editingFinished, this, [this, channelIndex, nameEdit]() {
        QString newName = nameEdit->text().trimmed();
        if (!newName.isEmpty() && newName != m_channels[channelIndex].name) {
            applyChannelNameChange(channelIndex, newName);
        }
        });

    connect(colorBtn, &QToolButton::clicked, this, [this, channelIndex, colorBtn]() {
        QColor currentColor = m_channels[channelIndex].color;

        QColor newColor = QColorDialog::getColor(
            currentColor,
            this,
            QString::fromLocal8Bit("选择通道颜色"),
            QColorDialog::ShowAlphaChannel
        );

        if (newColor.isValid() && newColor != currentColor) {
            applyChannelColorChange(channelIndex, newColor);
            updateColorButtonIcon(colorBtn, newColor);
        }
        });

    connect(zoomBtn, &QPushButton::clicked, this, [this, channelIndex]() {
        zoomChannel(channelIndex);
        });

    connect(removeBtn, &QPushButton::clicked, this, [this, channelIndex]() {
        removeChannel(channelIndex);
        });

    adjustPlotLayout();
}

/**
 * @brief 创建波形配置控件
 * @param channelBox 通道组框
 * @param channelIndex 通道索引
 */
void QtPlot::createWaveformConfigWidget(QGroupBox* channelBox, int channelIndex)
{
    if (channelIndex < 0 || channelIndex >= m_channels.size()) {
        qWarning() << "Invalid channel index in createWaveformConfigWidget:" << channelIndex;
        return;
    }

    ChannelData& channel = m_channels[channelIndex];

    QGridLayout* gridLayout = qobject_cast<QGridLayout*>(channelBox->layout());
    if (!gridLayout) {
        qWarning() << "Grid layout not found in channel box";
        return;
    }

    // 波形类型
    QLabel* waveTypeLabel = new QLabel(QString::fromLocal8Bit("波形:"), channelBox);
    waveTypeLabel->setStyleSheet("font-size: 12px; font-weight: bold;");
    waveTypeLabel->setObjectName("waveTypeLabel");

    QComboBox* waveTypeCombo = new QComboBox(channelBox);
    waveTypeCombo->setFixedHeight(28);
    waveTypeCombo->setStyleSheet(
        "QComboBox { "
        "font-size: 12px; "
        "padding: 2px; "
        "}"
    );
    waveTypeCombo->setObjectName("waveTypeCombo");
    waveTypeCombo->setToolTip(QString::fromLocal8Bit("选择波形类型"));
    waveTypeCombo->addItem(QString::fromLocal8Bit("正弦波"), SINE_WAVE);
    waveTypeCombo->addItem(QString::fromLocal8Bit("方波"), SQUARE_WAVE);
    waveTypeCombo->addItem(QString::fromLocal8Bit("三角波"), TRIANGLE_WAVE);
    waveTypeCombo->addItem(QString::fromLocal8Bit("锯齿波"), SAWTOOTH_WAVE);
    waveTypeCombo->addItem(QString::fromLocal8Bit("白噪声"), NOISE_WAVE);
    waveTypeCombo->setCurrentIndex(channel.waveform.type);
    waveTypeCombo->setProperty("channelIndex", channelIndex);

    // 频率
    QLabel* freqLabel = new QLabel(QString::fromLocal8Bit("频率(Hz):"), channelBox);
    freqLabel->setStyleSheet("font-size: 12px; font-weight: bold;");
    freqLabel->setObjectName("freqLabel");

    QDoubleSpinBox* freqSpin = new QDoubleSpinBox(channelBox);
    freqSpin->setFixedHeight(28);
    freqSpin->setStyleSheet(
        "QDoubleSpinBox { "
        "font-size: 12px; "
        "padding: 2px; "
        "}"
    );
    freqSpin->setObjectName("freqSpin");
    freqSpin->setToolTip(QString::fromLocal8Bit("设置波形频率"));
    freqSpin->setRange(0.01, 1000.0);
    freqSpin->setValue(channel.waveform.frequency);
    freqSpin->setSingleStep(0.1);
    freqSpin->setDecimals(2);
    freqSpin->setProperty("channelIndex", channelIndex);
    freqSpin->setSuffix(" Hz");

    // 幅值
    QLabel* ampLabel = new QLabel(QString::fromLocal8Bit("幅值:"), channelBox);
    ampLabel->setStyleSheet("font-size: 12px; font-weight: bold;");
    ampLabel->setObjectName("ampLabel");

    QDoubleSpinBox* ampSpin = new QDoubleSpinBox(channelBox);
    ampSpin->setFixedHeight(28);
    ampSpin->setStyleSheet(
        "QDoubleSpinBox { "
        "font-size: 12px; "
        "padding: 2px; "
        "}"
    );
    ampSpin->setObjectName("ampSpin");
    ampSpin->setToolTip(QString::fromLocal8Bit("设置波形幅值"));
    ampSpin->setRange(0.01, 10.0);
    ampSpin->setValue(channel.waveform.amplitude);
    ampSpin->setSingleStep(0.1);
    ampSpin->setDecimals(2);
    ampSpin->setProperty("channelIndex", channelIndex);

    // 偏置
    QLabel* offsetLabel = new QLabel(QString::fromLocal8Bit("偏置:"), channelBox);
    offsetLabel->setStyleSheet("font-size: 12px; font-weight: bold;");
    offsetLabel->setObjectName("offsetLabel");

    QDoubleSpinBox* offsetSpin = new QDoubleSpinBox(channelBox);
    offsetSpin->setFixedHeight(28);
    offsetSpin->setStyleSheet(
        "QDoubleSpinBox { "
        "font-size: 12px; "
        "padding: 2px; "
        "}"
    );
    offsetSpin->setObjectName("offsetSpin");
    offsetSpin->setToolTip(QString::fromLocal8Bit("设置波形偏置"));
    offsetSpin->setRange(-10.0, 10.0);
    offsetSpin->setValue(channel.waveform.offset);
    offsetSpin->setSingleStep(0.1);
    offsetSpin->setDecimals(2);
    offsetSpin->setProperty("channelIndex", channelIndex);

    // 相位
    QLabel* phaseLabel = new QLabel(QString::fromLocal8Bit("相位(°):"), channelBox);
    phaseLabel->setStyleSheet("font-size: 12px; font-weight: bold;");
    phaseLabel->setObjectName("phaseLabel");

    QDoubleSpinBox* phaseSpin = new QDoubleSpinBox(channelBox);
    phaseSpin->setFixedHeight(28);
    phaseSpin->setStyleSheet(
        "QDoubleSpinBox { "
        "font-size: 12px; "
        "padding: 2px; "
        "}"
    );
    phaseSpin->setObjectName("phaseSpin");
    phaseSpin->setToolTip(QString::fromLocal8Bit("设置波形相位"));
    phaseSpin->setRange(-180.0, 180.0);
    phaseSpin->setValue(channel.waveform.phase);
    phaseSpin->setSingleStep(1.0);
    phaseSpin->setDecimals(1);
    phaseSpin->setProperty("channelIndex", channelIndex);
    phaseSpin->setSuffix("");

    // 占空比（方波专用）
    QLabel* dutyLabel = new QLabel(QString::fromLocal8Bit("占空比(%):"), channelBox);
    dutyLabel->setStyleSheet("font-size: 12px; font-weight: bold;");
    dutyLabel->setObjectName("dutyLabel");

    QDoubleSpinBox* dutySpin = new QDoubleSpinBox(channelBox);
    dutySpin->setFixedHeight(28);
    dutySpin->setStyleSheet(
        "QDoubleSpinBox { "
        "font-size: 12px; "
        "padding: 2px; "
        "}"
    );
    dutySpin->setObjectName("dutySpin");
    dutySpin->setToolTip(QString::fromLocal8Bit("设置方波占空比"));
    dutySpin->setRange(1.0, 99.0);
    dutySpin->setValue(channel.waveform.dutyCycle * 100.0);
    dutySpin->setSingleStep(1.0);
    dutySpin->setDecimals(0);
    dutySpin->setProperty("channelIndex", channelIndex);
    dutySpin->setSuffix("%");

    // 占空比默认隐藏，只有方波时显示
    bool isSquareWave = (channel.waveform.type == SQUARE_WAVE);
    dutyLabel->setVisible(isSquareWave);
    dutySpin->setVisible(isSquareWave);

    // 应用按钮
    QPushButton* applyBtn = new QPushButton(QString::fromLocal8Bit("应用"), channelBox);
    applyBtn->setFixedHeight(28);
    applyBtn->setFixedWidth(60);
    applyBtn->setStyleSheet(
        "QPushButton { "
        "font-size: 12px; "
        "padding: 4px; "
        "background-color: #2ecc71; "
        "color: white; "
        "border: none; "
        "border-radius: 3px; "
        "}"
        "QPushButton:hover { "
        "background-color: #27ae60; "
        "}"
    );
    applyBtn->setObjectName("applyBtn");
    applyBtn->setToolTip(QString::fromLocal8Bit("应用波形配置"));
    applyBtn->setProperty("channelIndex", channelIndex);

    // 连接波形类型变化信号，控制占空比显示
    connect(waveTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, [dutyLabel, dutySpin, channelIndex](int index) {
            bool isSquareWave = (index == SQUARE_WAVE);
            dutyLabel->setVisible(isSquareWave);
            dutySpin->setVisible(isSquareWave);
           
        });

    // 连接应用按钮
    connect(applyBtn, &QPushButton::clicked, this, [this, channelIndex]() {
       
        applyWaveformConfig(channelIndex);
        });

    // 第3行
    int row = 2;
    gridLayout->addWidget(waveTypeLabel, row, 0, 1, 1);
    gridLayout->addWidget(waveTypeCombo, row, 1, 1, 2);
    gridLayout->addWidget(freqLabel, row, 3, 1, 1);
    gridLayout->addWidget(freqSpin, row, 4, 1, 2);

    // 第4行
    row = 3;
    gridLayout->addWidget(ampLabel, row, 0, 1, 1);
    gridLayout->addWidget(ampSpin, row, 1, 1, 2);
    gridLayout->addWidget(offsetLabel, row, 3, 1, 1);
    gridLayout->addWidget(offsetSpin, row, 4, 1, 2);

    // 第5行
    row = 4;
    gridLayout->addWidget(phaseLabel, row, 0, 1, 1);
    gridLayout->addWidget(phaseSpin, row, 1, 1, 2);
    gridLayout->addWidget(dutyLabel, row, 3, 1, 1);
    gridLayout->addWidget(dutySpin, row, 4, 1, 1);
    gridLayout->addWidget(applyBtn, row, 5, 1, 2);
}

/**
 * @brief 查找波形配置控件
 * @param channelBox 通道组框
 * @param waveTypeCombo [输出] 波形类型下拉框
 * @param freqSpin [输出] 频率调节框
 * @param ampSpin [输出] 幅值调节框
 * @param offsetSpin [输出] 偏置调节框
 * @param phaseSpin [输出] 相位调节框
 * @param dutySpin [输出] 占空比调节框
 * @return 是否找到所有必需控件
 */
bool QtPlot::findWaveformControls(QGroupBox* channelBox,
    QComboBox*& waveTypeCombo,
    QDoubleSpinBox*& freqSpin,
    QDoubleSpinBox*& ampSpin,
    QDoubleSpinBox*& offsetSpin,
    QDoubleSpinBox*& phaseSpin,
    QDoubleSpinBox*& dutySpin)
{
    if (!channelBox) {
        qWarning() << "Channel box is null in findWaveformControls";
        return false;
    }

    // 通过objectName直接查找控件
    waveTypeCombo = channelBox->findChild<QComboBox*>("waveTypeCombo");
    freqSpin = channelBox->findChild<QDoubleSpinBox*>("freqSpin");
    ampSpin = channelBox->findChild<QDoubleSpinBox*>("ampSpin");
    offsetSpin = channelBox->findChild<QDoubleSpinBox*>("offsetSpin");
    phaseSpin = channelBox->findChild<QDoubleSpinBox*>("phaseSpin");
    dutySpin = channelBox->findChild<QDoubleSpinBox*>("dutySpin");

    // 检查必要的控件是否找到
    bool allFound = true;

    if (!waveTypeCombo) {
        qWarning() << "waveTypeCombo not found";
        allFound = false;
    }
    if (!freqSpin) {
        qWarning() << "freqSpin not found";
        allFound = false;
    }
    if (!ampSpin) {
        qWarning() << "ampSpin not found";
        allFound = false;
    }
    if (!offsetSpin) {
        qWarning() << "offsetSpin not found";
        allFound = false;
    }
    if (!phaseSpin) {
        qWarning() << "phaseSpin not found";
        allFound = false;
    }

    // dutySpin是可选的，不检查

   

    return allFound;
}

/**
 * @brief 应用波形配置
 * @param channelIndex 通道索引
 */
void QtPlot::applyWaveformConfig(int channelIndex)
{
    

    if (channelIndex < 0 || channelIndex >= m_channels.size()) {
        qWarning() << "Invalid channel index:" << channelIndex;
        return;
    }

    ChannelData& channel = m_channels[channelIndex];

    // 找到通道控件
    if (channelIndex >= m_channelWidgets.size()) {
        qWarning() << "Channel widget not found for index:" << channelIndex;
        return;
    }

    QGroupBox* channelBox = qobject_cast<QGroupBox*>(m_channelWidgets[channelIndex]);
    if (!channelBox) {
        qWarning() << "Channel box is null for index:" << channelIndex;
        return;
    }

    // 查找所有波形配置控件
    QComboBox* waveTypeCombo = nullptr;
    QDoubleSpinBox* freqSpin = nullptr;
    QDoubleSpinBox* ampSpin = nullptr;
    QDoubleSpinBox* offsetSpin = nullptr;
    QDoubleSpinBox* phaseSpin = nullptr;
    QDoubleSpinBox* dutySpin = nullptr;

    if (!findWaveformControls(channelBox, waveTypeCombo, freqSpin, ampSpin, offsetSpin, phaseSpin, dutySpin)) {
        qWarning() << "Failed to find waveform controls for channel" << channelIndex;
        return;
    }

    // 获取新配置值
    WaveformType newType = static_cast<WaveformType>(waveTypeCombo->currentIndex());
    double newFrequency = freqSpin->value();
    double newAmplitude = ampSpin->value();
    double newOffset = offsetSpin->value();
    double newPhase = phaseSpin->value();
    double newDutyCycle = channel.waveform.dutyCycle;  // 默认使用之前的值

    // 获取占空比值（如果控件可见）
    if (dutySpin && dutySpin->isVisible()) {
        newDutyCycle = dutySpin->value() / 100.0;
       
    }
    else if (newType == SQUARE_WAVE) {
        // 如果是方波但占空比控件不可见，使用默认值
        newDutyCycle = 0.5;
       
    }

   

    // 检查值是否有变化
    bool hasChanged =
        (newType != channel.waveform.type) ||
        (qAbs(newFrequency - channel.waveform.frequency) > 0.0001) ||
        (qAbs(newAmplitude - channel.waveform.amplitude) > 0.0001) ||
        (qAbs(newOffset - channel.waveform.offset) > 0.0001) ||
        (qAbs(newPhase - channel.waveform.phase) > 0.1) ||
        (newType == SQUARE_WAVE && qAbs(newDutyCycle - channel.waveform.dutyCycle) > 0.001);

    if (!hasChanged) {
        qDebug() << "No changes detected, skipping update";
        return;
    }

    // 更新波形配置
    channel.waveform.type = newType;
    channel.waveform.frequency = newFrequency;
    channel.waveform.amplitude = newAmplitude;
    channel.waveform.offset = newOffset;
    channel.waveform.phase = newPhase;
    channel.waveform.dutyCycle = newDutyCycle;
    // 清空历史数据，从新波形开始
    clearChannelPlot(channelIndex);

    // 更新通道名称显示波形类型
    QString waveTypeStr = getWaveformTypeName(channel.waveform.type);

    QString newName = QString::fromLocal8Bit("%1(%2Hz)").arg(waveTypeStr).arg(channel.waveform.frequency, 0, 'f', 2);
    applyChannelNameChange(channelIndex, newName);

    // 更新控件
    QLineEdit* nameEdit = channelBox->findChild<QLineEdit*>("nameEdit");
    if (nameEdit) {
        nameEdit->setText(newName);
    }

    // 更新图形中的波形信息
    if (channelIndex < m_plots.size()) {
        QCustomPlot* plot = m_plots[channelIndex];
        if (plot) {
            QCPTextElement* waveText = qobject_cast<QCPTextElement*>(
                plot->plotLayout()->element(1, 0));
            if (waveText) {
                QString waveInfo = QString::fromLocal8Bit("波形: %1  %2Hz").arg(waveTypeStr).arg(channel.waveform.frequency, 0, 'f', 2);
                waveText->setText(waveInfo);
            }

            // 调整Y轴范围以适应新波形
            double yMin = channel.waveform.offset - channel.waveform.amplitude * 1.5;
            double yMax = channel.waveform.offset + channel.waveform.amplitude * 1.5;
            plot->yAxis->setRange(yMin, yMax);

            plot->replot();
        }
    }

    qDebug() << "Waveform config applied successfully for channel" << channelIndex;
}

/**
 * @brief 生成正弦波
 * @param time 时间
 * @param config 波形配置
 * @return 正弦波值
 */
double QtPlot::generateSineWave(double time, const WaveformConfig& config)
{
    double phaseRad = qDegreesToRadians(config.phase);
    return config.amplitude * qSin(2.0 * M_PI * config.frequency * time + phaseRad) + config.offset;
}

/**
 * @brief 生成方波
 * @param time 时间
 * @param config 波形配置
 * @return 方波值
 */
double QtPlot::generateSquareWave(double time, const WaveformConfig& config)
{
    double t = fmod(config.frequency * time + config.phase / 360.0, 1.0);
    if (t < 0) t += 1.0;

    if (t < config.dutyCycle) {
        return config.amplitude + config.offset;
    }
    else {
        return -config.amplitude + config.offset;
    }
}

/**
 * @brief 生成三角波
 * @param time 时间
 * @param config 波形配置
 * @return 三角波值
 */
double QtPlot::generateTriangleWave(double time, const WaveformConfig& config)
{
    double t = fmod(config.frequency * time + config.phase / 360.0, 1.0);
    if (t < 0) t += 1.0;

    if (t < 0.25) {
        return 4.0 * t * config.amplitude + config.offset;
    }
    else if (t < 0.75) {
        return 2.0 * config.amplitude * (1.0 - 2.0 * t) + config.offset;
    }
    else {
        return 4.0 * config.amplitude * (t - 1.0) + config.offset;
    }
}

/**
 * @brief 生成锯齿波
 * @param time 时间
 * @param config 波形配置
 * @return 锯齿波值
 */
double QtPlot::generateSawtoothWave(double time, const WaveformConfig& config)
{
    double t = fmod(config.frequency * time + config.phase / 360.0, 1.0);
    if (t < 0) t += 1.0;

    return 2.0 * config.amplitude * (t - 0.5) + config.offset;
}

/**
 * @brief 生成白噪声
 * @return 随机噪声值
 */
double QtPlot::generateNoiseWave()
{
    // 生成 [-1, 1] 之间的随机数
    return (m_randomGenerator.bounded(200) - 100) * 0.01;
}

/**
 * @brief 生成波形
 * @param time 时间
 * @param config 波形配置
 * @return 波形值
 */
double QtPlot::generateWaveform(double time, const WaveformConfig& config)
{
    switch (config.type) {
    case SINE_WAVE:
        return generateSineWave(time, config);
    case SQUARE_WAVE:
        return generateSquareWave(time, config);
    case TRIANGLE_WAVE:
        return generateTriangleWave(time, config);
    case SAWTOOTH_WAVE:
        return generateSawtoothWave(time, config);
    case NOISE_WAVE:
        return generateNoiseWave() * config.amplitude + config.offset;
    default:
        return 0.0;
    }
}

/**
 * @brief 获取波形类型名称
 * @param type 波形类型
 * @return 波形类型名称
 */
QString QtPlot::getWaveformTypeName(WaveformType type)
{
    switch (type) {
    case SINE_WAVE: return QString::fromLocal8Bit("正弦波");
    case SQUARE_WAVE: return QString::fromLocal8Bit("方波");
    case TRIANGLE_WAVE: return QString::fromLocal8Bit("三角波");
    case SAWTOOTH_WAVE: return QString::fromLocal8Bit("锯齿波");
    case NOISE_WAVE: return QString::fromLocal8Bit("白噪声");
    default: return QString::fromLocal8Bit("未知");
    }
}

/**
 * @brief 获取实时数据
 * @param channelIndex 通道索引
 * @return 实时数据值
 */
double QtPlot::getRealtimeData(int channelIndex)
{
    if (channelIndex < 0 || channelIndex >= m_channels.size()) {
        qWarning() << "Invalid channel index in getRealtimeData:" << channelIndex;
        return 0.0;
    }

    ChannelData& channel = m_channels[channelIndex];
    double time = m_elapsedSeconds;

    double value = generateWaveform(time, channel.waveform);

   

    return value;
}

/**
 * @brief 移除指定通道
 * @param channelIndex 通道索引
 */
void QtPlot::removeChannel(int channelIndex)
{
    qDebug() << "removeChannel called for index:" << channelIndex;

    if (channelIndex < 0 || channelIndex >= m_channels.size()) {
        qWarning() << "Invalid channel index:" << channelIndex;
        return;
    }

    if (channelIndex < m_plots.size()) {
        if (m_plots[channelIndex]) {
            m_plotLayout->removeWidget(m_plots[channelIndex]);
            m_plots[channelIndex]->setParent(nullptr);
            m_plots[channelIndex]->deleteLater();
        }
        m_plots.removeAt(channelIndex);
    }

    if (channelIndex < m_channelWidgets.size()) {
        if (m_channelWidgets[channelIndex]) {
            m_channelControlLayout->removeWidget(m_channelWidgets[channelIndex]);
            m_channelWidgets[channelIndex]->setParent(nullptr);
            m_channelWidgets[channelIndex]->deleteLater();
        }
        m_channelWidgets.removeAt(channelIndex);
    }

    m_channels.removeAt(channelIndex);

    // 重新连接信号
    for (int i = channelIndex; i < m_channels.size(); ++i) {
        m_channels[i].id = i;

        if (i < m_channelWidgets.size()) {
            QGroupBox* channelBox = qobject_cast<QGroupBox*>(m_channelWidgets[i]);
            if (channelBox) {
                // 重新连接所有信号
                QCheckBox* visibleCheck = channelBox->findChild<QCheckBox*>("visibleCheck");
                QPushButton* removeBtn = channelBox->findChild<QPushButton*>("removeBtn");
                QLineEdit* nameEdit = channelBox->findChild<QLineEdit*>("nameEdit");
                QToolButton* colorBtn = channelBox->findChild<QToolButton*>("colorBtn");
                QPushButton* zoomBtn = channelBox->findChild<QPushButton*>("zoomBtn");
                QPushButton* applyBtn = channelBox->findChild<QPushButton*>("applyBtn");

                if (visibleCheck && removeBtn && nameEdit && colorBtn && zoomBtn && applyBtn) {
                    // 断开所有旧的连接
                    channelBox->disconnect();
                    visibleCheck->disconnect();
                    removeBtn->disconnect();
                    nameEdit->disconnect();
                    colorBtn->disconnect();
                    zoomBtn->disconnect();
                    applyBtn->disconnect();

                    // 重新连接信号
                    connect(channelBox, &QGroupBox::toggled, this, [this, i](bool checked) {
                        m_channels[i].visible = checked;
                        if (i < m_plots.size()) {
                            m_plots[i]->setVisible(checked);
                        }
                        adjustPlotLayout();
                        });

                    connect(visibleCheck, &QCheckBox::toggled, this, [this, i](bool checked) {
                        m_channels[i].visible = checked;
                        if (i < m_plots.size()) {
                            m_plots[i]->setVisible(checked);
                        }
                        adjustPlotLayout();
                        });

                    connect(nameEdit, &QLineEdit::editingFinished, this, [this, i, nameEdit]() {
                        QString newName = nameEdit->text().trimmed();
                        if (!newName.isEmpty() && newName != m_channels[i].name) {
                            applyChannelNameChange(i, newName);
                        }
                        });

                    connect(colorBtn, &QToolButton::clicked, this, [this, i, colorBtn]() {
                        QColor currentColor = m_channels[i].color;
                        QColor newColor = QColorDialog::getColor(
                            currentColor,
                            this,
                            QString::fromLocal8Bit("选择通道颜色"),
                            QColorDialog::ShowAlphaChannel
                        );

                        if (newColor.isValid() && newColor != currentColor) {
                            applyChannelColorChange(i, newColor);
                            updateColorButtonIcon(colorBtn, newColor);
                        }
                        });

                    connect(zoomBtn, &QPushButton::clicked, this, [this, i]() {
                        zoomChannel(i);
                        });

                    connect(applyBtn, &QPushButton::clicked, this, [this, i]() {
                        qDebug() << "Reconnected apply button for channel" << i;
                        applyWaveformConfig(i);
                        });

                    connect(removeBtn, &QPushButton::clicked, this, [this, i]() {
                        removeChannel(i);
                        });
                }
            }
        }
    }

    adjustPlotLayout();

}

/**
 * @brief 创建通道绘图
 * @param channelIndex 通道索引
 */
 /**
  * @brief 创建通道绘图
  * @param channelIndex 通道索引
  */
void QtPlot::createChannelPlot(int channelIndex)
{
    qDebug() << "createChannelPlot called for channel" << channelIndex;

    if (channelIndex < 0 || channelIndex >= m_channels.size()) {
        qWarning() << "Invalid channel index:" << channelIndex;
        return;
    }

    ChannelData& channel = m_channels[channelIndex];

    QCustomPlot* plot = new QCustomPlot(m_plotContainer);

    int plotHeight = 300;
    plot->setMinimumHeight(plotHeight);
    plot->setMaximumHeight(plotHeight);

    plot->setBackground(QBrush(QColor(245, 245, 245)));

    // 添加图形
    plot->addGraph();
    plot->graph(0)->setPen(QPen(channel.color, 2.5));
    plot->graph(0)->setName(channel.name);
    plot->graph(0)->setBrush(Qt::NoBrush);
    plot->graph(0)->setAntialiased(true);  // 开启抗锯齿

    // 简化X轴配置
    plot->xAxis->setVisible(false);
    plot->xAxis->setRange(0, 500);  // 固定显示500个点

    // 固定Y轴范围
    plot->yAxis->setVisible(true);
    plot->yAxis->setLabel(QString::fromLocal8Bit("数值"));
    plot->yAxis->setLabelFont(QFont("Microsoft YaHei", 10));
    plot->yAxis->setTickLabelFont(QFont("Microsoft YaHei", 9));
    plot->yAxis->setRange(-10.0, 10.0);  // 固定范围-10到10

    plot->yAxis->setBasePen(QPen(Qt::black, 1.5));
    plot->yAxis->setTickPen(QPen(Qt::black, 1.5));
    plot->yAxis->setSubTickPen(QPen(Qt::black, 1));
    plot->yAxis->setTickLabelColor(Qt::black);
    plot->yAxis->setLabelColor(Qt::black);

    // 网格线
    plot->xAxis->grid()->setPen(QPen(QColor(200, 200, 200), 1.2, Qt::DotLine));
    plot->yAxis->grid()->setPen(QPen(QColor(200, 200, 200), 1.2, Qt::DotLine));
    plot->xAxis->grid()->setSubGridVisible(false);
    plot->yAxis->grid()->setSubGridVisible(false);

    // 标题
    QCPTextElement* title = new QCPTextElement(plot, channel.name, QFont("Microsoft YaHei", 11, QFont::Bold));
    title->setTextColor(channel.color);
    plot->plotLayout()->insertRow(0);
    plot->plotLayout()->addElement(0, 0, title);

    // 波形信息显示
    QString waveTypeStr = getWaveformTypeName(channel.waveform.type);
    QString waveInfo = QString::fromLocal8Bit("波形: %1  %2Hz").arg(waveTypeStr).arg(channel.waveform.frequency, 0, 'f', 2);
    QCPTextElement* waveText = new QCPTextElement(plot, waveInfo, QFont("Microsoft YaHei", 9));
    waveText->setTextColor(Qt::darkBlue);
    plot->plotLayout()->insertRow(1);
    plot->plotLayout()->addElement(1, 0, waveText);

    // 峰峰值显示
    QCPTextElement* peakText = new QCPTextElement(plot,
        QString::fromLocal8Bit("峰峰值: 0.000"),
        QFont("Microsoft YaHei", 9));
    peakText->setTextColor(Qt::darkRed);
    plot->plotLayout()->insertRow(2);
    plot->plotLayout()->addElement(2, 0, peakText);
    peakText->setProperty("channelIndex", channelIndex);

    // 交互设置
    plot->setInteraction(QCP::iRangeDrag, false);
    plot->setInteraction(QCP::iRangeZoom, false);

    // 性能优化
    plot->setNoAntialiasingOnDrag(true);
    plot->setNotAntialiasedElement(QCP::aeAll);

    // 开启OpenGL加速（如果可用）
#ifdef QCUSTOMPLOT_USE_OPENGL
    plot->setOpenGl(true);
#endif

    m_plots.append(plot);
    m_plotLayout->addWidget(plot);

    resetChannelData(channelIndex);

}
/**
 * @brief 更新通道数据
 * @param channelIndex 通道索引
 */
 /**
  * @brief 更新通道数据
  * @param channelIndex 通道索引
  */
void QtPlot::updateChannelData(int channelIndex)
{
    if (channelIndex < 0 || channelIndex >= m_channels.size() ||
        channelIndex >= m_plots.size()) {
        return;
    }

    ChannelData& channel = m_channels[channelIndex];
    QCustomPlot* plot = m_plots[channelIndex];

    if (!plot || !channel.visible) {
        return;
    }

    // 获取新数据
    double newData = getRealtimeData(channelIndex);

    // 添加到数据缓冲区
    channel.data.append(newData);

    // 如果数据太多，删除旧数据
    if (channel.data.size() > m_maxDataPoints) {
        int removeCount = channel.data.size() - m_maxDataPoints;
        channel.data.remove(0, removeCount);
    }

    // 准备X坐标（简单的索引）
    int dataSize = channel.data.size();
    QVector<double> xData(dataSize);
    for (int i = 0; i < dataSize; ++i) {
        xData[i] = i;  // 简单的索引作为X坐标
    }

    // 更新图形数据
    plot->graph(0)->setData(xData, channel.data, true);

    // 自动滚动X轴
    if (dataSize > 500) {
        plot->xAxis->setRange(dataSize - 500, dataSize);
    }
    else {
        plot->xAxis->setRange(0, 500);
    }

    // 计算峰峰值
    calculatePeakToPeak(channelIndex);

    // 更新峰峰值显示
    updatePeakToPeakDisplay(channelIndex);

    // 使用排队重绘
    plot->replot(QCustomPlot::rpQueuedReplot);

    // 更新控件显示
    if (channelIndex < m_channelWidgets.size()) {
        QGroupBox* channelBox = qobject_cast<QGroupBox*>(m_channelWidgets[channelIndex]);
        if (channelBox) {
            // 更新数据点数
            QLabel* dataCountLabel = channelBox->findChild<QLabel*>("dataCountLabel");
            if (dataCountLabel) {
                dataCountLabel->setText(QString::number(channel.data.size()));
            }

            // 更新峰峰值显示
            QLabel* peakValueLabel = channelBox->findChild<QLabel*>("peakValueLabel");
            if (peakValueLabel) {
                peakValueLabel->setText(QString::number(channel.peakToPeak, 'f', 3));
            }
        }
    }

    m_dataUpdateCount++;
}

/**
 * @brief 计算峰峰值
 * @param channelIndex 通道索引
 */
void QtPlot::calculatePeakToPeak(int channelIndex)
{
    if (channelIndex < 0 || channelIndex >= m_channels.size()) {
        qWarning() << "Invalid channel index in calculatePeakToPeak:" << channelIndex;
        return;
    }

    ChannelData& channel = m_channels[channelIndex];

    if (channel.data.isEmpty()) {
        channel.peakToPeak = 0.0;
        channel.minValue = 0.0;
        channel.maxValue = 0.0;
        return;
    }

    // 计算最新100个数据点的峰峰值（滑动窗口）
    int windowSize = qMin(100, channel.data.size());
    int startIdx = channel.data.size() - windowSize;

    double minVal = channel.data[startIdx];
    double maxVal = channel.data[startIdx];

    for (int i = startIdx; i < channel.data.size(); ++i) {
        double val = channel.data[i];
        if (val < minVal) minVal = val;
        if (val > maxVal) maxVal = val;
    }

    channel.minValue = minVal;
    channel.maxValue = maxVal;
    channel.peakToPeak = maxVal - minVal;
}

/**
 * @brief 更新峰峰值显示
 * @param channelIndex 通道索引
 */
void QtPlot::updatePeakToPeakDisplay(int channelIndex)
{
    if (channelIndex < 0 || channelIndex >= m_plots.size()) {
        qWarning() << "Invalid channel index in updatePeakToPeakDisplay:" << channelIndex;
        return;
    }

    QCustomPlot* plot = m_plots[channelIndex];
    if (!plot) {
        qWarning() << "Plot is null for channel" << channelIndex;
        return;
    }

    ChannelData& channel = m_channels[channelIndex];

    // 更新波形信息
    QCPTextElement* waveText = qobject_cast<QCPTextElement*>(
        plot->plotLayout()->element(1, 0));

    if (waveText) {
        QString waveTypeStr = getWaveformTypeName(channel.waveform.type);
        QString waveInfo = QString::fromLocal8Bit("波形: %1  %2Hz").arg(waveTypeStr).arg(channel.waveform.frequency, 0, 'f', 2);
        waveText->setText(waveInfo);
    }

    // 更新峰峰值显示
    QCPTextElement* peakText = qobject_cast<QCPTextElement*>(
        plot->plotLayout()->element(2, 0));

    if (peakText) {
        QString peakStr = QString::fromLocal8Bit("峰峰值: %1")
            .arg(channel.peakToPeak, 0, 'f', 3);

        // 根据峰峰值大小设置颜色
        if (channel.peakToPeak > channel.waveform.amplitude * 1.8) {
            peakText->setTextColor(Qt::red);
        }
        else if (channel.peakToPeak > channel.waveform.amplitude * 0.8) {
            peakText->setTextColor(QColor(255, 140, 0));
        }
        else {
            peakText->setTextColor(Qt::darkGreen);
        }

        peakText->setText(peakStr);
    }

    // 在图形上显示最大值和最小值标记
    if (plot->graphCount() > 0) {
        // 清除旧的标注
        plot->clearItems();

        if (!channel.data.isEmpty() && channel.peakToPeak > 0.01) {
            // 找到最大值和最小值的位置
            double maxPos = 0;
            double minPos = 0;

            for (int i = 0; i < channel.data.size(); ++i) {
                if (qFuzzyCompare(channel.data[i], channel.maxValue)) {
                    maxPos = i * m_updateInterval * 0.001;
                }
                if (qFuzzyCompare(channel.data[i], channel.minValue)) {
                    minPos = i * m_updateInterval * 0.001;
                }
            }

            // 添加最大值标记
            QCPItemText* maxText = new QCPItemText(plot);
            maxText->setPositionAlignment(Qt::AlignBottom | Qt::AlignHCenter);
            maxText->position->setType(QCPItemPosition::ptPlotCoords);
            maxText->position->setCoords(maxPos, channel.maxValue);
            maxText->setText(QString::fromLocal8Bit("MAX:%1").arg(channel.maxValue, 0, 'f', 3));
            maxText->setFont(QFont("Microsoft YaHei", 8));
            maxText->setColor(channel.color.darker(150));

            // 添加最小值标记
            QCPItemText* minText = new QCPItemText(plot);
            minText->setPositionAlignment(Qt::AlignTop | Qt::AlignHCenter);
            minText->position->setType(QCPItemPosition::ptPlotCoords);
            minText->position->setCoords(minPos, channel.minValue);
            minText->setText(QString::fromLocal8Bit("MIN:%1").arg(channel.minValue, 0, 'f', 3));
            minText->setFont(QFont("Microsoft YaHei", 8));
            minText->setColor(channel.color.darker(150));
        }
    }
}

/**
 * @brief 放大指定通道
 * @param channelIndex 通道索引
 */
void QtPlot::zoomChannel(int channelIndex)
{
    qDebug() << "zoomChannel called for channel" << channelIndex;

    if (channelIndex < 0 || channelIndex >= m_channels.size()) {
        qWarning() << "Invalid channel index:" << channelIndex;
        return;
    }

    // 如果已经在放大模式，先退出
    if (m_zoomChannelIndex >= 0) {
        exitZoomMode();
    }

    m_zoomChannelIndex = channelIndex;
    m_channels[channelIndex].zoomMode = true;

    // 显示退出放大按钮
    m_exitZoomBtn->setVisible(true);
    m_exitZoomBtn->setText(QString::fromLocal8Bit("退出放大模式 (通道%1)").arg(channelIndex + 1));

    // 隐藏所有其他通道
    for (int i = 0; i < m_channels.size(); ++i) {
        if (i != channelIndex) {
            m_channels[i].visible = false;
            if (i < m_plots.size()) {
                m_plots[i]->setVisible(false);
            }
        }
        else {
            m_channels[i].visible = true;
            if (i < m_plots.size()) {
                m_plots[i]->setVisible(true);
                int plotHeight = 1000;
                m_plots[i]->setMinimumHeight(plotHeight);
                m_plots[i]->setMaximumHeight(plotHeight);
            }
        }
    }

    // 更新所有放大按钮状态
    updateAllZoomButtons();

    adjustPlotLayout();
    qDebug() << "Channel" << channelIndex << "zoomed";
}

/**
 * @brief 退出放大模式
 */
void QtPlot::exitZoomMode()
{
    qDebug() << "exitZoomMode called, current zoom channel:" << m_zoomChannelIndex;

    if (m_zoomChannelIndex < 0) {
        qWarning() << "Not in zoom mode";
        return;
    }

    int zoomedChannel = m_zoomChannelIndex;
    m_zoomChannelIndex = -1;

    // 隐藏退出放大按钮
    m_exitZoomBtn->setVisible(false);

    // 恢复所有通道的显示状态
    for (int i = 0; i < m_channels.size(); ++i) {
        m_channels[i].zoomMode = false;
        m_channels[i].visible = true;
        if (i < m_plots.size()) {
            m_plots[i]->setVisible(true);
            int plotHeight = 300;
            m_plots[i]->setMinimumHeight(plotHeight);
            m_plots[i]->setMaximumHeight(plotHeight);
        }
    }

    // 恢复放大按钮文本
    for (int i = 0; i < m_channelWidgets.size(); ++i) {
        QGroupBox* channelBox = qobject_cast<QGroupBox*>(m_channelWidgets[i]);
        if (channelBox) {
            QPushButton* zoomBtn = channelBox->findChild<QPushButton*>("zoomBtn");
            if (zoomBtn && zoomBtn->text() == QString::fromLocal8Bit("已放大")) {
                zoomBtn->setText(QString::fromLocal8Bit("放大"));
            }
        }
    }

    // 更新所有放大按钮状态
    updateAllZoomButtons();

    adjustPlotLayout();
    qDebug() << "Exited zoom mode, previous zoom channel:" << zoomedChannel;
}

/**
 * @brief 更新所有放大按钮状态
 */
void QtPlot::updateAllZoomButtons()
{
    for (int i = 0; i < m_channelWidgets.size(); ++i) {
        QGroupBox* channelBox = qobject_cast<QGroupBox*>(m_channelWidgets[i]);
        if (channelBox) {
            QPushButton* zoomBtn = channelBox->findChild<QPushButton*>("zoomBtn");
            if (!zoomBtn) continue;

            if (m_zoomChannelIndex == i) {
                // 当前通道被放大
                zoomBtn->setText(QString::fromLocal8Bit("已放大"));
                zoomBtn->setStyleSheet(
                    "QPushButton { "
                    "font-size: 12px; "
                    "padding: 4px; "
                    "background-color: #27ae60; "
                    "color: white; "
                    "border: none; "
                    "border-radius: 3px; "
                    "}"
                );
                zoomBtn->setEnabled(true);
            }
            else if (m_zoomChannelIndex >= 0) {
                // 其他通道被放大
                zoomBtn->setText(QString::fromLocal8Bit("放大"));
                zoomBtn->setStyleSheet(
                    "QPushButton { "
                    "font-size: 12px; "
                    "padding: 4px; "
                    "background-color: #95a5a6; "
                    "color: white; "
                    "border: none; "
                    "border-radius: 3px; "
                    "}"
                );
                zoomBtn->setEnabled(false);
            }
            else {
                // 正常模式
                zoomBtn->setText(QString::fromLocal8Bit("放大"));
                zoomBtn->setStyleSheet(
                    "QPushButton { "
                    "font-size: 12px; "
                    "padding: 4px; "
                    "background-color: #3498db; "
                    "color: white; "
                    "border: none; "
                    "border-radius: 3px; "
                    "}"
                    "QPushButton:hover { "
                    "background-color: #2980b9; "
                    "}"
                );
                zoomBtn->setEnabled(true);
            }
        }
    }
}

/**
 * @brief 更新所有通道数据
 */
void QtPlot::updateData()
{
    static int updateCounter = 0;

    if (updateCounter++ % 10 == 0) {
        m_currentTime++;
    }

    for (int i = 0; i < m_channels.size(); ++i) {
        if (m_channels[i].visible) {
            updateChannelData(i);
        }
    }
}

/**
 * @brief 更新时间显示
 */
void QtPlot::updateTimeDisplay()
{
    int hours = static_cast<int>(m_elapsedSeconds) / 3600;
    int minutes = (static_cast<int>(m_elapsedSeconds) % 3600) / 60;
    int seconds = static_cast<int>(m_elapsedSeconds) % 60;
    int milliseconds = static_cast<int>((m_elapsedSeconds - static_cast<int>(m_elapsedSeconds)) * 1000);

    m_runTimeLabel->setText(QString::fromLocal8Bit("运行时间: %1:%2:%3.%4")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'))
        .arg(milliseconds, 3, 10, QChar('0')));

    QDateTime currentTime = QDateTime::currentDateTime();
    m_currentTimeLabel->setText(currentTime.toString("yyyy-MM-dd HH:mm:ss.zzz"));

    static QTime lastUpdateTime = QTime::currentTime();
    QTime currentUpdateTime = QTime::currentTime();
    int elapsedMs = lastUpdateTime.msecsTo(currentUpdateTime);

    if (elapsedMs >= 1000) {
        double dataRate = m_dataUpdateCount * 1000.0 / elapsedMs;
        m_dataRateLabel->setText(QString::fromLocal8Bit("数据速率: %1 点/秒")
            .arg(static_cast<int>(dataRate)));
        m_dataUpdateCount = 0;
        lastUpdateTime = currentUpdateTime;
    }
}

/**
 * @brief 调整绘图布局
 */
void QtPlot::adjustPlotLayout()
{
    for (int i = 0; i < m_plots.size() && i < m_channels.size(); ++i) {
        if (m_channels[i].visible) {
            m_plots[i]->show();
        }
        else {
            m_plots[i]->hide();
        }
    }

    m_plotContainer->adjustSize();
}

/**
 * @brief 清空通道绘图
 * @param channelIndex 通道索引
 */
void QtPlot::clearChannelPlot(int channelIndex)
{
    qDebug() << "clearChannelPlot called for channel" << channelIndex;

    if (channelIndex < 0 || channelIndex >= m_channels.size() ||
        channelIndex >= m_plots.size()) {
        qWarning() << "Invalid channel index:" << channelIndex;
        return;
    }

    ChannelData& channel = m_channels[channelIndex];
    QCustomPlot* plot = m_plots[channelIndex];

    if (!plot) {
        qWarning() << "Plot is null for channel" << channelIndex;
        return;
    }

    channel.data.clear();
    channel.peakToPeak = 0.0;
    channel.minValue = 0.0;
    channel.maxValue = 0.0;
    plot->graph(0)->data()->clear();
    plot->clearItems();
    plot->replot();

    // 更新峰峰值显示
    updatePeakToPeakDisplay(channelIndex);

    qDebug() << "Channel plot cleared for channel" << channelIndex;
}

/**
 * @brief 重置通道数据
 * @param channelIndex 通道索引
 */
void QtPlot::resetChannelData(int channelIndex)
{
    qDebug() << "resetChannelData called for channel" << channelIndex;

    if (channelIndex < 0 || channelIndex >= m_channels.size()) {
        qWarning() << "Invalid channel index:" << channelIndex;
        return;
    }

    ChannelData& channel = m_channels[channelIndex];

    channel.data.clear();
    channel.needClear = false;
    channel.peakToPeak = 0.0;
    channel.minValue = 0.0;
    channel.maxValue = 0.0;

    // 生成100个初始数据点
    for (int i = 0; i < 100; ++i) {
        double time = i * 0.1;
        double value = generateWaveform(time, channel.waveform);
        channel.data.append(value);
    }

}

/**
 * @brief 重置所有时间
 */
void QtPlot::resetAllTime()
{
    qDebug() << "resetAllTime called";

    m_elapsedSeconds = 0.0;
    m_currentTime = 0;
    updateTimeDisplay();

    for (int i = 0; i < m_channels.size(); ++i) {
        clearChannelPlot(i);
    }

    m_dataUpdateCount = 0;

    qDebug() << "All time reset";
}

/**
 * @brief 清空所有通道
 */
void QtPlot::clearAllChannels()
{
    qDebug() << "clearAllChannels called";

    toggleDataUpdate(false);
    resetAllTime();

    while (!m_channels.isEmpty()) {
        removeChannel(0);
    }

    qDebug() << "All channels cleared";
}

/**
 * @brief 设置通道数量
 * @param count 目标通道数量
 */
void QtPlot::setChannelCount(int count)
{
    qDebug() << "setChannelCount called, target count:" << count
        << "current count:" << m_channels.size();

    int currentCount = m_channels.size();

    if (count > currentCount) {
        for (int i = currentCount; i < count; ++i) {
            addChannel();
        }
    }
    else if (count < currentCount) {
        for (int i = currentCount - 1; i >= count; --i) {
            removeChannel(i);
        }
    }

    qDebug() << "Channel count set to" << m_channels.size();
}

/**
 * @brief 应用通道名称变更
 * @param channelIndex 通道索引
 * @param newName 新名称
 */
void QtPlot::applyChannelNameChange(int channelIndex, const QString& newName)
{
    qDebug() << "applyChannelNameChange called for channel" << channelIndex << "new name:" << newName;

    if (channelIndex < 0 || channelIndex >= m_channels.size()) {
        qWarning() << "Invalid channel index:" << channelIndex;
        return;
    }

    m_channels[channelIndex].name = newName;

    if (channelIndex < m_channelWidgets.size()) {
        QGroupBox* channelBox = qobject_cast<QGroupBox*>(m_channelWidgets[channelIndex]);
        if (channelBox) {
            updateChannelBoxStyle(channelBox, m_channels[channelIndex].color, newName);

            QLineEdit* nameEdit = channelBox->findChild<QLineEdit*>("nameEdit");
            if (nameEdit) {
                nameEdit->setText(newName);
            }
        }
    }

    if (channelIndex < m_plots.size()) {
        QCustomPlot* plot = m_plots[channelIndex];
        if (plot) {
            if (plot->graphCount() > 0) {
                plot->graph(0)->setName(newName);
            }

            QCPTextElement* title = qobject_cast<QCPTextElement*>(
                plot->plotLayout()->element(0, 0));
            if (title) {
                title->setText(newName);
                title->setTextColor(m_channels[channelIndex].color);
            }

            plot->replot();
        }
    }

    qDebug() << "Channel name changed for channel" << channelIndex;
}

/**
 * @brief 应用通道颜色变更
 * @param channelIndex 通道索引
 * @param newColor 新颜色
 */
void QtPlot::applyChannelColorChange(int channelIndex, const QColor& newColor)
{
    qDebug() << "applyChannelColorChange called for channel" << channelIndex << "new color:" << newColor;

    if (channelIndex < 0 || channelIndex >= m_channels.size()) {
        qWarning() << "Invalid channel index:" << channelIndex;
        return;
    }

    m_channels[channelIndex].color = newColor;

    if (channelIndex < m_channelWidgets.size()) {
        QGroupBox* channelBox = qobject_cast<QGroupBox*>(m_channelWidgets[channelIndex]);
        if (channelBox) {
            updateChannelBoxStyle(channelBox, newColor, m_channels[channelIndex].name);

            QToolButton* colorBtn = channelBox->findChild<QToolButton*>("colorBtn");
            if (colorBtn) {
                updateColorButtonIcon(colorBtn, newColor);
            }
        }
    }

    if (channelIndex < m_plots.size()) {
        QCustomPlot* plot = m_plots[channelIndex];
        if (plot) {
            if (plot->graphCount() > 0) {
                plot->graph(0)->setPen(QPen(newColor, 2.5));
            }

            QCPTextElement* title = qobject_cast<QCPTextElement*>(
                plot->plotLayout()->element(0, 0));
            if (title) {
                title->setTextColor(newColor);
            }

            plot->replot();
        }
    }

    qDebug() << "Channel color changed for channel" << channelIndex;
}

/**
 * @brief 更新通道框样式
 * @param channelBox 通道组框
 * @param color 颜色
 * @param name 名称
 */
void QtPlot::updateChannelBoxStyle(QGroupBox* channelBox, const QColor& color, const QString& name)
{
    if (!channelBox) {
        qWarning() << "Channel box is null in updateChannelBoxStyle";
        return;
    }

    channelBox->setTitle(name);

    QColor backgroundColor = color.lighter(180);
    if (backgroundColor.lightness() > 240) {
        backgroundColor = backgroundColor.darker(110);
    }

    QColor borderColor = color.darker(150);
    QColor textColor = (color.lightness() > 128) ? color.darker(250) : color.lighter(180);

    QString styleSheet = QString(
        "QGroupBox { "
        "border: 2px solid %1; "
        "border-radius: 6px; "
        "margin-top: 0.6em; "
        "background-color: %2; "
        "padding-top: 10px; "
        "}"
        "QGroupBox::title { "
        "subcontrol-origin: margin; "
        "left: 12px; "
        "padding: 0 6px 0 6px; "
        "color: black; "
        "font-weight: bold; "
        "font-size: 11px; "
        "}"
    ).arg(borderColor.name()).arg(backgroundColor.name());

    channelBox->setStyleSheet(styleSheet);

    qDebug() << "Channel box style updated for:" << name;
}

/**
 * @brief 更新颜色按钮图标
 * @param button 颜色按钮
 * @param color 颜色
 */
void QtPlot::updateColorButtonIcon(QToolButton* button, const QColor& color)
{
    if (!button) {
        qWarning() << "Color button is null in updateColorButtonIcon";
        return;
    }

    QPixmap pixmap(24, 24);
    pixmap.fill(color);

    QPainter painter(&pixmap);
    painter.setPen(QPen(Qt::black, 2));
    painter.drawRect(1, 1, pixmap.width() - 3, pixmap.height() - 3);

    button->setIcon(QIcon(pixmap));
    button->setIconSize(QSize(20, 20));

    qDebug() << "Color button icon updated to color:" << color;
}

/**
 * @brief 获取通道颜色
 * @param index 通道索引
 * @return 通道颜色
 */
QColor QtPlot::getChannelColor(int index)
{
    static const QVector<QColor> colors = {
        QColor(255, 0, 0),      // 红色
        QColor(0, 150, 0),      // 深绿色
        QColor(0, 0, 255),      // 蓝色
        QColor(255, 128, 0),    // 橙色
        QColor(128, 0, 128),    // 紫色
        QColor(0, 128, 128),    // 青色
        QColor(255, 0, 255),    // 洋红
        QColor(128, 128, 0),    // 橄榄色
        QColor(0, 0, 128),      // 深蓝
        QColor(139, 69, 19),    // 棕色
        QColor(255, 20, 147),   // 深粉色
        QColor(0, 255, 127),    // 春绿色
        QColor(70, 130, 180),   // 钢蓝色
        QColor(255, 140, 0),    // 深橙色
        QColor(186, 85, 211)    // 中兰花紫
    };

    QColor color = colors.at(index % colors.size());
    qDebug() << "getChannelColor for index" << index << "returning:" << color;
    return color;
}

/**
 * @brief 设置更新间隔
 * @param interval 更新间隔（毫秒）
 */
void QtPlot::setUpdateInterval(int interval)
{
    qDebug() << "setUpdateInterval called, new interval:" << interval << "ms";

    m_updateInterval = interval;
    if (m_updateTimer) {
        m_updateTimer->setInterval(interval);
    }
}

/**
 * @brief 切换数据更新状态
 * @param start true开始更新，false停止更新
 */
void QtPlot::toggleDataUpdate(bool start)
{
    qDebug() << "toggleDataUpdate called, start:" << start;

    m_isUpdating = start;

    if (m_updateTimer) {
        if (start) {
            m_startTime = QDateTime::currentDateTime();
            m_updateTimer->start();
            m_runTimeTimer->start();
            qDebug() << "Data update started";
        }
        else {
            m_updateTimer->stop();
            m_runTimeTimer->stop();
            resetAllTime();
            qDebug() << "Data update stopped";
        }
    }
}

/**
 * @brief 切换全屏状态
 */
void QtPlot::toggleFullScreen()
{
    qDebug() << "toggleFullScreen called, current state:" << m_isFullScreen;

    if (m_isFullScreen) {
        showNormal();
        m_isFullScreen = false;
        qDebug() << "Switched to windowed mode";
    }
    else {
        showFullScreen();
        m_isFullScreen = true;
        qDebug() << "Switched to full screen mode";
    }
}