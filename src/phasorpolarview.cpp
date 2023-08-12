#include "phasorpolarview.h"

PhasorPolarView::PhasorPolarView(QWidget* parent)
    : QChartView(parent) {

    hide();

    auto chart = new QPolarChart();
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

    auto axisAngular = new QCategoryAxis(this);
    axisAngular->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
    axisAngular->setRange(0, 360);
    for (qint16 angleOnAxis = 0; angleOnAxis < 360; angleOnAxis += 30) {
        auto label = QString::asprintf("%d&deg;", toAngleActual(angleOnAxis));
        axisAngular->append(label, angleOnAxis);
    }
    chart->addAxis(axisAngular, QPolarChart::PolarOrientationAngular);

    auto axisRadial = new QValueAxis(this);
    axisRadial->setRange(0, 300);
    chart->addAxis(axisRadial, QPolarChart::PolarOrientationRadial);

    for (auto phasor : Phasor::phasors) {
        auto series = new QLineSeries(this);
        chart->addSeries(series);

        auto values = phasor->values();
        series->append(0.0, 0.0);
        if (values.size()) {
            series->append(toPointF(values.back()));
        }

        series->setPen(QPen(QBrush(phasor->color), 2.0));
        series->attachAxis(axisAngular);
        series->attachAxis(axisRadial);

        connect(
            phasor,
            &Phasor::newValueAdded,
            this,
            [series](const Phasor::Value& v) {
                if (series->count() == 1) {
                    series->append(toPointF(v));
                } else {
                    series->replace(1, toPointF(v));
                }
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

QPointF PhasorPolarView::toPointF(const Phasor::Value& value) {
    return {toAngleOnAxisF(value.phaseAngle * 360.0), value.magnitude};
}

constexpr qint32 PhasorPolarView::normal(qint32 angle) {
    return ((angle % 360) + 360) % 360;
}

constexpr qreal PhasorPolarView::normalF(qreal angle) {
    angle = fmod(angle, 360.0);
    return angle + ((angle < 0.0) * 360.0);
}

constexpr qint32 PhasorPolarView::toAngleOnAxis(qint32 angleActual) {
    return normal(-angleActual - 90);
}

constexpr qreal PhasorPolarView::toAngleOnAxisF(qreal angleActual) {
    return normalF(-angleActual - 90);
}

constexpr qint32 PhasorPolarView::toAngleActual(qint32 angleOnAxis) {
    return normal(-angleOnAxis + 90);
}

constexpr qreal PhasorPolarView::toAngleActualF(qreal angleOnAxis) {
    return normalF(-angleOnAxis + 90.0);
}
