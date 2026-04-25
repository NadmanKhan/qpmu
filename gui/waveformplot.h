#pragma once

#include <QWidget>

class SignalDataModel;

class WaveformPlot : public QWidget
{
    Q_OBJECT

public:
    explicit WaveformPlot(SignalDataModel *model, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void drawGrid(QPainter &p);
    void drawWaveforms(QPainter &p);
    void drawAxesLabels(QPainter &p);
    void drawTooltipLabels(QPainter &p);

    QRectF chartRect() const;
    qreal dataToScreenX(qreal t) const;
    qreal dataToScreenY(qreal y, bool isVoltage) const;

    int hitTestWaveform(const QPointF &pos) const;

    SignalDataModel *m_model;

    static constexpr int POINTS_PER_CYCLE = 40;
    static constexpr int CYCLE_COUNT = 1;
    static constexpr qreal CHART_WIDTH_RATIO = 0.78;
    static constexpr qreal CHART_HEIGHT_RATIO = 0.73;
    static constexpr int GRID_ROW_COUNT = 9;
    static constexpr int GRID_DIVS_PER_CYCLE = 4;
    static constexpr qreal LINE_WIDTH = 2.5;
    static constexpr qreal VERTICAL_SCALE = 0.5;
    static constexpr qreal HIT_DISTANCE = 24.0;
};
