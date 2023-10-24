#include "abstractphasorview.h"

constexpr auto cssTemplateWhite =
    "QCheckBox::indicator {"
    "    background-color: white;"
    "}"
    "QCheckBox::indicator:checked {"
    "    border-image: url(:/images/view.png) 0 0 0 0 stretch stretch;"
    "}";

constexpr auto cssTemplateColorful =
    "QCheckBox::indicator {"
    "    background-color: %1;"
    "}"
    "QCheckBox::indicator:checked {"
    "    border-image: url(:/images/view.png) 0 0 0 0 stretch stretch;"
    "}";

AbstractPhasorView::AbstractPhasorView(QChart* chart, QWidget* parent)
    : QChartView(chart, parent) {

    hide();
    setRenderHint(QPainter::Antialiasing, true);

    chart->setAnimationOptions(QChart::NoAnimation);
    chart->legend()->hide();
    chart->setTheme(QChart::ChartThemeBlueIcy);

    m_controlLayout = new QVBoxLayout();

    m_controlWidget = new QWidget();
    m_controlWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_controlWidget->setLayout(m_controlLayout);

    auto allVoltagesCheckBox = new QCheckBox("All Voltages", this);
    allVoltagesCheckBox->setChecked(true);
    allVoltagesCheckBox->setStyleSheet(cssTemplateWhite);
    connect(allVoltagesCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        for (auto& x : m_controlCheckBoxes) {
            if (x.second == PMU::Voltage) x.first->setChecked(checked);
        }
    });
    m_controlLayout->addWidget(allVoltagesCheckBox, 1);

    auto allCurrentsCheckBox = new QCheckBox("All Currents", this);
    allCurrentsCheckBox->setChecked(true);
    allCurrentsCheckBox->setStyleSheet(cssTemplateWhite);
    connect(allCurrentsCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        for (auto& x : m_controlCheckBoxes) {
            if (x.second == PMU::Current) x.first->setChecked(checked);
        }
    });
    m_controlLayout->addWidget(allCurrentsCheckBox, 1);

    tb = AppToolBar::ptr();
    dw = AppDockWidget::ptr();
    pmu = PMU::ptr();
}

AbstractPhasorView::~AbstractPhasorView() {
    delete m_controlLayout;
    delete m_controlWidget;
}

void AbstractPhasorView::addSeriesToControl(QXYSeries* series, PMU::PhasorType phasorType) {
    auto checkBox = new QCheckBox(series->name(), this);

    auto css = QString(cssTemplateColorful)
                   .arg(series->pen().color().name(QColor::HexRgb));
    checkBox->setStyleSheet(css);

    checkBox->setChecked(true);
    connect(checkBox, &QCheckBox::toggled, series, &QXYSeries::setVisible);

    m_controlCheckBoxes.append(qMakePair(checkBox, phasorType));
    m_controlLayout->addWidget(checkBox, 0);

    for (int i = 0; i < (int)PMU::NumChannels; ++i) {
        m_series.emplace_back(nullptr);
        m_pointsWindow.emplace_back(QList<QPointF>{});
    }
}

void AbstractPhasorView::showEvent([[maybe_unused]] QShowEvent* event) {
    dw->setWidget(m_controlWidget);
    tb->setControlEnabled(true);
}

void AbstractPhasorView::hideEvent([[maybe_unused]] QHideEvent* event) {
    tb->setControlChecked(false);
    tb->setControlEnabled(false);
}
