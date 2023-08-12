#include "phasorwaveview.h"

PhasorWaveView::PhasorWaveView(QWidget* parent)
    : QChartView(parent) {

    hide();

    auto chart = new QChart();
    setChart(chart);
    setRenderHint(QPainter::Antialiasing, true);

    chart->setAnimationOptions(QChart::NoAnimation);
    chart->legend()->hide();
    chart->setTheme(QChart::ChartThemeBlueIcy);

    auto controlWidget = new QWidget();
    controlWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    auto controlWidgetLayout = new QVBoxLayout();
    controlWidget->setLayout(controlWidgetLayout);

    auto cw = AppCentralWidget::ptr();
    auto tb = AppToolBar::ptr();
    connect(cw,
            &AppCentralWidget::currentChanged,
            tb,
            [this, cw, tb, controlWidget](int index) {
                if (cw->widget(index) == this) {
                    // this is the active widget
                    tb->setControlWidget(controlWidget);
                } else {
                    // this is NOT the active widget
                    tb->setControlWidget(nullptr);
                }
            });

    auto axisX = new QValueAxis();
    axisX->setTickCount(Phasor::capacity() + 1);
    axisX->setLabelsVisible(false);
    chart->addAxis(axisX, Qt::AlignBottom);

    auto axisY = new QValueAxis();
    axisY->setRange(-Phasor::maxAmplitude(), +Phasor::maxAmplitude());
    chart->addAxis(axisY, Qt::AlignLeft);

    auto rangeWidthX = Phasor::interval() * Phasor::capacity();

    for (int i = 0; i < (int)Phasor::phasors.size(); ++i) {
        const auto phasor = Phasor::phasors[i];
        auto values = phasor->values();
        auto minValue = values.front();

        auto series = new QLineSeries(this);
        chart->addSeries(series);

        series->setName(phasor->name);
        series->setPen(QPen(QBrush(phasor->color), 2.0));
        series->attachAxis(axisX);
        series->attachAxis(axisY);

        for (auto v : values) series->append(toPoint(v));
        connect(
            phasor,
            &Phasor::newValueAdded,
            this,
            [series, axisX, rangeWidthX, minValue](const Phasor::Value& v) {
                auto maxX = v.timeStamp;
                auto minX = qMax(minValue.timeStamp, maxX - rangeWidthX);
                axisX->setRange(minX, maxX);
                series->append(toPoint(v));
            });

        auto checkBox = new QCheckBox(phasor->label, controlWidget);
        {
            auto css = QString(
                           "QCheckBox::indicator {"
                           "    background-color: %1;"
                           "}"
                           "QCheckBox::indicator:checked {"
                           "    border-image: url(:/images/view.png) 0 0 0 0 stretch stretch;"
                           "}")
                           .arg(series->pen().color().name(QColor::HexRgb));
            checkBox->setStyleSheet(css);

            checkBox->setChecked(true);
            connect(checkBox, &QCheckBox::toggled, series, &QLineSeries::setVisible);
        }
        controlWidgetLayout->addWidget(checkBox, 0);
    }
}

QPointF PhasorWaveView::toPoint(const Phasor::Value& value) {
    return {(qreal)value.timeStamp, value.magnitude};
}
