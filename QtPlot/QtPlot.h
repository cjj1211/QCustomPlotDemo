#ifndef QTPLOT_H
#define QTPLOT_H

#include <QMainWindow>
#include <qcustomplot.h>
#include <QVector>
#include <QList>
#include <QTimer>
#include <QRandomGenerator>

QT_BEGIN_NAMESPACE
class QCustomPlot;
class QVBoxLayout;
class QCheckBox;
class QPushButton;
class QSpinBox;
class QGroupBox;
class QLabel;
class QTimer;
class QScrollArea;
class QLineEdit;
class QToolButton;
class QComboBox;
class QDoubleSpinBox;
QT_END_NAMESPACE

/**
 * @enum WaveformType
 * @brief 波形类型枚举
 *
 * 定义了五种基本的波形类型
 */
    enum WaveformType {
    SINE_WAVE = 0,      ///< 正弦波
    SQUARE_WAVE = 1,    ///< 方波
    TRIANGLE_WAVE = 2,  ///< 三角波
    SAWTOOTH_WAVE = 3,  ///< 锯齿波
    NOISE_WAVE = 4      ///< 白噪声
};

/**
 * @struct WaveformConfig
 * @brief 波形配置结构体
 *
 * 存储波形生成参数
 */
struct WaveformConfig {
    WaveformType type;      ///< 波形类型
    double frequency;       ///< 频率 (Hz)
    double amplitude;       ///< 幅值
    double offset;          ///< 偏置
    double phase;           ///< 相位 (度)
    double dutyCycle;       ///< 占空比 (0-1，方波专用)
};

/**
 * @struct ChannelData
 * @brief 通道数据结构体
 *
 * 存储单个通道的数据和配置信息
 */
struct ChannelData {
    int id;                    ///< 通道ID
    QString name;              ///< 通道名称
    QVector<double> data;      ///< 数据缓冲区
    QColor color;              ///< 显示颜色
    bool visible;              ///< 是否可见
    int maxDataPoints;         ///< 最大数据点数
    bool needClear;            ///< 是否需要清屏
    bool zoomMode;             ///< 是否处于放大模式
    double peakToPeak;         ///< 峰峰值
    double minValue;           ///< 最小值
    double maxValue;           ///< 最大值
    WaveformConfig waveform;   ///< 波形配置
};

/**
 * @class QtPlot
 * @brief 多通道实时数据监控系统
 *
 * 每个通道在独立的子图中垂直布局显示
 * 支持通道放大、峰峰值测量、实时数据更新、波形配置等功能
 */
class QtPlot : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     */
    explicit QtPlot(QWidget* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~QtPlot();

    /**
     * @brief 窗口显示事件
     * @param event 显示事件
     */
    void showEvent(QShowEvent* event) override;

public slots:
    /**
     * @brief 添加通道
     */
    void addChannel();

    /**
     * @brief 移除指定通道
     * @param channelIndex 通道索引
     */
    void removeChannel(int channelIndex);

    /**
     * @brief 清空所有通道
     */
    void clearAllChannels();

    /**
     * @brief 设置通道数量
     * @param count 目标通道数量
     */
    void setChannelCount(int count);

    /**
     * @brief 更新所有通道数据
     */
    void updateData();

private slots:
    /**
     * @brief 设置更新间隔
     * @param interval 更新间隔（毫秒）
     */
    void setUpdateInterval(int interval);

    /**
     * @brief 切换数据更新状态
     * @param start true开始更新，false停止更新
     */
    void toggleDataUpdate(bool start);

    /**
     * @brief 切换全屏状态
     */
    void toggleFullScreen();

    /**
     * @brief 更新时间显示
     */
    void updateTimeDisplay();

    /**
     * @brief 放大指定通道
     * @param channelIndex 通道索引
     */
    void zoomChannel(int channelIndex);

    /**
     * @brief 退出放大模式
     */
    void exitZoomMode();

    /**
     * @brief 应用波形配置
     * @param channelIndex 通道索引
     */
    void applyWaveformConfig(int channelIndex);

private:
    /**
     * @brief 设置用户界面
     */
    void setupUI();

    /**
     * @brief 设置绘图区域
     */
    void setupPlot();

    /**
     * @brief 设置信号槽连接
     */
    void setupConnections();

    /**
     * @brief 获取通道颜色
     * @param index 通道索引
     * @return 通道颜色
     */
    QColor getChannelColor(int index);

    /**
     * @brief 创建通道绘图
     * @param channelIndex 通道索引
     */
    void createChannelPlot(int channelIndex);

    /**
     * @brief 更新通道数据
     * @param channelIndex 通道索引
     */
    void updateChannelData(int channelIndex);

    /**
     * @brief 调整绘图布局
     */
    void adjustPlotLayout();

    /**
     * @brief 获取实时数据
     * @param channelIndex 通道索引
     * @return 实时数据
     */
    double getRealtimeData(int channelIndex);

    /**
     * @brief 清空通道绘图
     * @param channelIndex 通道索引
     */
    void clearChannelPlot(int channelIndex);

    /**
     * @brief 重置通道数据
     * @param channelIndex 通道索引
     */
    void resetChannelData(int channelIndex);

    /**
     * @brief 重置所有时间
     */
    void resetAllTime();

    /**
     * @brief 应用通道名称变更
     * @param channelIndex 通道索引
     * @param newName 新名称
     */
    void applyChannelNameChange(int channelIndex, const QString& newName);

    /**
     * @brief 应用通道颜色变更
     * @param channelIndex 通道索引
     * @param newColor 新颜色
     */
    void applyChannelColorChange(int channelIndex, const QColor& newColor);

    /**
     * @brief 更新通道框样式
     * @param channelBox 通道组框
     * @param color 颜色
     * @param name 名称
     */
    void updateChannelBoxStyle(QGroupBox* channelBox, const QColor& color, const QString& name);

    /**
     * @brief 更新颜色按钮图标
     * @param button 颜色按钮
     * @param color 颜色
     */
    void updateColorButtonIcon(QToolButton* button, const QColor& color);

    /**
     * @brief 计算峰峰值
     * @param channelIndex 通道索引
     */
    void calculatePeakToPeak(int channelIndex);

    /**
     * @brief 更新峰峰值显示
     * @param channelIndex 通道索引
     */
    void updatePeakToPeakDisplay(int channelIndex);

    /**
     * @brief 更新所有放大按钮状态
     */
    void updateAllZoomButtons();

    // 波形生成函数

    /**
     * @brief 生成正弦波
     * @param time 时间
     * @param config 波形配置
     * @return 波形值
     */
    double generateSineWave(double time, const WaveformConfig& config);

    /**
     * @brief 生成方波
     * @param time 时间
     * @param config 波形配置
     * @return 波形值
     */
    double generateSquareWave(double time, const WaveformConfig& config);

    /**
     * @brief 生成三角波
     * @param time 时间
     * @param config 波形配置
     * @return 波形值
     */
    double generateTriangleWave(double time, const WaveformConfig& config);

    /**
     * @brief 生成锯齿波
     * @param time 时间
     * @param config 波形配置
     * @return 波形值
     */
    double generateSawtoothWave(double time, const WaveformConfig& config);

    /**
     * @brief 生成白噪声
     * @return 随机噪声值
     */
    double generateNoiseWave();

    /**
     * @brief 生成波形
     * @param time 时间
     * @param config 波形配置
     * @return 波形值
     */
    double generateWaveform(double time, const WaveformConfig& config);

    /**
     * @brief 获取波形类型名称
     * @param type 波形类型
     * @return 波形类型名称
     */
    QString getWaveformTypeName(WaveformType type);

    /**
     * @brief 创建波形配置控件
     * @param channelBox 通道组框
     * @param channelIndex 通道索引
     */
    void createWaveformConfigWidget(QGroupBox* channelBox, int channelIndex);

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
    bool findWaveformControls(QGroupBox* channelBox,
        QComboBox*& waveTypeCombo,
        QDoubleSpinBox*& freqSpin,
        QDoubleSpinBox*& ampSpin,
        QDoubleSpinBox*& offsetSpin,
        QDoubleSpinBox*& phaseSpin,
        QDoubleSpinBox*& dutySpin);

private:
    QWidget* m_plotContainer;           ///< 绘图容器
    QVBoxLayout* m_mainLayout;          ///< 主垂直布局
    QVBoxLayout* m_plotLayout;          ///< 绘图垂直布局
    QVBoxLayout* m_channelControlLayout; ///< 通道控制布局

    QList<QCustomPlot*> m_plots;        ///< 子图列表
    QList<ChannelData> m_channels;      ///< 通道数据列表
    QList<QWidget*> m_channelWidgets;   ///< 通道控件列表

    QTimer* m_updateTimer;              ///< 数据更新定时器
    QTimer* m_currentTimeTimer;         ///< 当前时间显示定时器
    QTimer* m_runTimeTimer;             ///< 运行时间定时器

    QLabel* m_runTimeLabel;             ///< 运行时间显示标签
    QLabel* m_currentTimeLabel;         ///< 当前时间标签
    QLabel* m_dataRateLabel;            ///< 数据速率标签

    int m_currentTime;                  ///< 当前时间索引
    int m_maxDataPoints;                ///< 每个通道最大数据点数
    int m_updateInterval;               ///< 更新间隔（毫秒）
    bool m_isUpdating;                  ///< 是否正在更新数据
    double m_elapsedSeconds;            ///< 经过的秒数
    bool m_isFullScreen;                ///< 是否全屏
    QDateTime m_startTime;              ///< 开始时间
    int m_dataUpdateCount;              ///< 数据更新计数
    int m_zoomChannelIndex;             ///< 当前放大的通道索引，-1表示无放大
    QPushButton* m_exitZoomBtn;         ///< 退出放大按钮

    QRandomGenerator m_randomGenerator; ///< 随机数生成器
};

#endif // QTPLOT_H