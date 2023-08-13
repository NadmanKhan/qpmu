#include "abstractphasorview.h"

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

    tb = AppToolBar::ptr();
    dw = AppDockWidget::ptr();
}

AbstractPhasorView::~AbstractPhasorView() {
    delete m_controlLayout;
    delete m_controlWidget;
}

constexpr auto cssTemplate =
    "QCheckBox::indicator {"
    "    background-color: %1;"
    "}"
    "QCheckBox::indicator:checked {"
    "    border-image: url(:/images/view.png) 0 0 0 0 stretch stretch;"
    "}";

void AbstractPhasorView::addSeriesToControl(QXYSeries* series) {
    auto checkBox = new QCheckBox(series->name(), this);

    auto css = QString(cssTemplate)
                   .arg(series->pen().color().name(QColor::HexRgb));
    checkBox->setStyleSheet(css);

    checkBox->setChecked(true);
    connect(checkBox, &QCheckBox::toggled, series, &QXYSeries::setVisible);

    m_controlLayout->addWidget(checkBox, 0);
}

void AbstractPhasorView::showEvent([[maybe_unused]] QShowEvent* event) {
    dw->setWidget(m_controlWidget);
    tb->setControlEnabled(true);
}

void AbstractPhasorView::hideEvent([[maybe_unused]] QHideEvent* event) {
    tb->setControlChecked(false);
    tb->setControlEnabled(false);
}
