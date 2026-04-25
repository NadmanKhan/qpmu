#include "livemonitorscreen.h"
#include "signaldatamodel.h"
#include "contextitemmodel.h"
#include "phasorplot.h"
#include "waveformplot.h"
#include "theme.h"

#include <QVBoxLayout>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabBar>
#include <QTableView>
#include <QHeaderView>
#include <QStyledItemDelegate>
#include <QPainter>

// ---------------------------------------------------------------------------
// Table delegate (moved from mainwindow.cpp)
// ---------------------------------------------------------------------------

class SignalTableDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        painter->save();

        QColor sigColor = index.data(Qt::DecorationRole).value<QColor>();
        bool selected = option.state & QStyle::State_Selected;

        QColor bg = Theme::withAlpha(sigColor, selected ? 77 : 25);
        painter->fillRect(option.rect, bg);

        if (selected) {
            painter->setPen(QPen(sigColor, Theme::Border::medium));
            painter->drawRect(option.rect.adjusted(1, 1, -1, -1));
        }

        QFont font(Theme::Font::monospace, Theme::Font::normal);
        font.setWeight(selected ? QFont::DemiBold : QFont::Normal);
        painter->setFont(font);
        painter->setPen(selected ? Theme::Colors::textPrimary : sigColor);

        QString text = index.data(Qt::DisplayRole).toString();
        QRect textRect = option.rect.adjusted(Theme::Spacing::small, 0, -Theme::Spacing::small, 0);
        painter->drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, text);

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override
    {
        return QSize(120, Theme::Sizing::small + Theme::Spacing::small);
    }
};

// ---------------------------------------------------------------------------
// LiveMonitorScreen
// ---------------------------------------------------------------------------

LiveMonitorScreen::LiveMonitorScreen(SignalDataModel *model, QWidget *parent)
    : Screen(parent), m_model(model)
{
    m_phasorPlot = new PhasorPlot(m_model);
    m_waveformPlot = new WaveformPlot(m_model);

    m_tabBar = new QTabBar;
    m_tabBar->addTab(QStringLiteral("◉ Phasor"));
    m_tabBar->addTab(QStringLiteral("∿ Waveform"));
    m_tabBar->setDocumentMode(true);
    m_tabBar->setExpanding(false);
    m_tabBar->setStyleSheet(
        QStringLiteral(
            "QTabBar { background: %1; }"
            "QTabBar::tab { background: %2; color: %3; padding: 8px 16px;"
            " border-radius: %4px; margin: 4px 2px; font-weight: 600; font-size: %5px; }"
            "QTabBar::tab:selected { background: %6; color: %7; border-bottom: 2px solid %8; }")
            .arg(Theme::Colors::surface.name(), Theme::Colors::surfaceElevated.name(),
                 Theme::Colors::textTertiary.name())
            .arg(Theme::Radius::medium)
            .arg(Theme::Font::normal)
            .arg(Theme::Colors::borderEmphasized.name(), Theme::Colors::textPrimary.name(),
                 Theme::Colors::primary.name()));

    m_plotStack = new QStackedWidget;
    m_plotStack->addWidget(m_phasorPlot);
    m_plotStack->addWidget(m_waveformPlot);
    connect(m_tabBar, &QTabBar::currentChanged, m_plotStack, &QStackedWidget::setCurrentIndex);

    auto *graphPane = new QWidget;
    auto *graphLayout = new QVBoxLayout(graphPane);
    graphLayout->setContentsMargins(0, 0, 0, 0);
    graphLayout->setSpacing(0);
    graphLayout->addWidget(m_tabBar);
    graphLayout->addWidget(m_plotStack, 1);

    m_tableView = new QTableView;
    m_tableView->setModel(m_model);
    m_tableView->setSelectionModel(m_model->selectionModel());
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::MultiSelection);
    m_tableView->setItemDelegate(new SignalTableDelegate(m_tableView));
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->verticalHeader()->setDefaultSectionSize(
        Theme::Sizing::small + Theme::Spacing::small);
    m_tableView->setShowGrid(false);
    m_tableView->setStyleSheet(
        QStringLiteral(
            "QTableView { background: %1; color: %2; gridline-color: %3;"
            " font-family: '%4'; font-size: %5px; border: none; }"
            "QHeaderView::section { background: %6; color: %7; padding: 6px;"
            " border: none; border-bottom: 1px solid %8; font-weight: 600; font-size: %9px; }")
            .arg(Theme::Colors::surface.name(), Theme::Colors::textPrimary.name(),
                 Theme::Colors::borderSubtle.name(), Theme::Font::monospace)
            .arg(Theme::Font::normal)
            .arg(Theme::Colors::surfaceElevated.name(), Theme::Colors::textSecondary.name(),
                 Theme::Colors::borderEmphasized.name())
            .arg(Theme::Font::small));

    m_splitter = new QSplitter(Qt::Horizontal);
    m_splitter->addWidget(graphPane);
    m_splitter->addWidget(m_tableView);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setHandleWidth(6);
    m_splitter->setStyleSheet(
        QStringLiteral("QSplitter::handle { background: %1; }"
                       "QSplitter::handle:hover { background: %2; }")
            .arg(Theme::Colors::surfaceElevated.name(),
                 Theme::Colors::borderEmphasized.name()));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_splitter);

    buildContextModel();
}

QString LiveMonitorScreen::title() const
{
    return QStringLiteral("Live Monitor");
}

ContextItemModel *LiveMonitorScreen::contextModel() const
{
    return m_contextModel;
}

// ---------------------------------------------------------------------------
// Context model construction
// ---------------------------------------------------------------------------

void LiveMonitorScreen::buildContextModel()
{
    m_contextModel = new ContextItemModel(this);

    int dataSection = m_contextModel->addSection(QStringLiteral("DATA CONTROLS"));

    m_contextModel->addToggle(
        dataSection, QStringLiteral("Magnitude"),
        { QStringLiteral("RMS"), QStringLiteral("Peak") },
        [this]() -> QVariant {
            return m_model->magnitudeMode() == SignalDataModel::Peak;
        },
        [this](const QVariant &v) {
            m_model->setMagnitudeMode(v.toBool() ? SignalDataModel::Peak : SignalDataModel::RMS);
        });

    m_contextModel->addDropdown(
        dataSection, QStringLiteral("Phase Reference"),
        { QStringLiteral("Absolute"),
          QStringLiteral("VA"), QStringLiteral("VB"), QStringLiteral("VC"),
          QStringLiteral("IA"), QStringLiteral("IB"), QStringLiteral("IC") },
        [this]() -> QVariant {
            return m_model->phaseReferenceIndex() + 1;
        },
        [this](const QVariant &v) {
            m_model->setPhaseReferenceIndex(v.toInt() - 1);
        });

    m_contextModel->addButton(
        dataSection, QStringLiteral("Pause"),
        QStringLiteral("⏸"),
        [this]() -> QVariant { return m_model->isPaused(); },
        [this](const QVariant &) {
            m_model->setIsPaused(!m_model->isPaused());
        });

    int plotSection = m_contextModel->addSection(QStringLiteral("PLOT CONTROLS"));

    m_contextModel->addToggle(
        plotSection, QStringLiteral("Voltage Scaling"),
        { QStringLiteral("Dynamic"), QStringLiteral("Manual") },
        [this]() -> QVariant {
            return m_model->voltageScalingMode() == SignalDataModel::Manual;
        },
        [this](const QVariant &v) {
            m_model->setVoltageScalingMode(
                v.toBool() ? SignalDataModel::Manual : SignalDataModel::Dynamic);
        });

    m_contextModel->addSlider(
        plotSection, QStringLiteral("Voltage Cutoff"),
        10, 500, 5, 0, QStringLiteral("V"),
        [this]() -> QVariant { return m_model->voltageCutoff(); },
        [this](const QVariant &v) { m_model->setVoltageCutoff(v.toReal()); });

    m_contextModel->addToggle(
        plotSection, QStringLiteral("Current Scaling"),
        { QStringLiteral("Dynamic"), QStringLiteral("Manual") },
        [this]() -> QVariant {
            return m_model->currentScalingMode() == SignalDataModel::Manual;
        },
        [this](const QVariant &v) {
            m_model->setCurrentScalingMode(
                v.toBool() ? SignalDataModel::Manual : SignalDataModel::Dynamic);
        });

    m_contextModel->addSlider(
        plotSection, QStringLiteral("Current Cutoff"),
        1, 50, 0.5, 1, QStringLiteral("A"),
        [this]() -> QVariant { return m_model->currentCutoff(); },
        [this](const QVariant &v) { m_model->setCurrentCutoff(v.toReal()); });

    // Sync when model properties change
    auto refresh = [this]() { m_contextModel->refreshValues(); };
    connect(m_model, &SignalDataModel::magnitudeModeChanged, this, refresh);
    connect(m_model, &SignalDataModel::phaseReferenceChanged, this, refresh);
    connect(m_model, &SignalDataModel::pauseStateChanged, this, refresh);
    connect(m_model, &SignalDataModel::voltageScalingChanged, this, refresh);
    connect(m_model, &SignalDataModel::currentScalingChanged, this, refresh);
}
