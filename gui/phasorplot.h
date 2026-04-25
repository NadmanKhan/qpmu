#pragma once

#include <QWidget>

class SignalDataModel;

class PhasorPlot : public QWidget
{
    Q_OBJECT

public:
    explicit PhasorPlot(SignalDataModel *model, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void drawGrid(QPainter &p);
    void drawPhasors(QPainter &p);
    void drawAngleLabels(QPainter &p);
    void drawTooltipLabels(QPainter &p);

    QPointF phasorTip(int signalIndex) const;
    QPointF center() const;
    qreal plotRadius() const;

    int hitTestPhasor(const QPointF &pos) const;
    static qreal pointToSegmentDist(const QPointF &pt, const QPointF &a, const QPointF &b);

    SignalDataModel *m_model;

    static constexpr qreal RADIUS_RATIO = 0.38;
    static constexpr qreal ARROW_SIZE = 12.0;
    static constexpr qreal ARROW_ANGLE_DEG = 20.0;
    static constexpr qreal PHASOR_LINE_WIDTH = 3.0;
    static constexpr qreal GRID_LINE_THIN = 1.2;
    static constexpr qreal GRID_LINE_THICK = 2.5;
    static constexpr qreal CENTER_DOT_RADIUS = 5.0;
    static constexpr qreal HIT_RADIUS = 24.0;
    static constexpr qreal LABEL_DISTANCE_RATIO = 1.15;
};
