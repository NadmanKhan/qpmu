#include "mainwindow.h"
#include "signaldatamodel.h"
#include "phasorplot.h"
#include "waveformplot.h"
#include "contextmenupanel.h"
#include "theme.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabBar>
#include <QTableView>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QFrame>
#include <QResizeEvent>

// ---------------------------------------------------------------------------
// Table delegate: colored cell backgrounds based on signal color + selection
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

        // Cell background
        QColor bg = Theme::withAlpha(sigColor, selected ? 77 : 25); // 30% / 10%
        painter->fillRect(option.rect, bg);

        // Selection border
        if (selected) {
            painter->setPen(QPen(sigColor, Theme::Border::medium));
            painter->drawRect(option.rect.adjusted(1, 1, -1, -1));
        }

        // Text
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
// MainWindow
// ---------------------------------------------------------------------------

MainWindow::MainWindow(SignalDataModel *model, QWidget *parent)
    : QMainWindow(parent), m_model(model)
{
    setWindowTitle(QStringLiteral("QPMU - Phasor Measurement Unit"));
    resize(1280, 800);

    auto *central = new QWidget;
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *toolbar = new QWidget(central);
    setupToolbar();
    toolbar->setObjectName(QStringLiteral("toolbar"));
    toolbar->setFixedHeight(Theme::Sizing::toolbarHeight);
    toolbar->setStyleSheet(
            QStringLiteral("QWidget#toolbar { background: %1; border-bottom: 1px solid %2; }")
                    .arg(Theme::Colors::surface.name(), Theme::Colors::borderEmphasized.name()));
    {
        auto *hbox = new QHBoxLayout(toolbar);
        hbox->setContentsMargins(Theme::Spacing::small, 0, Theme::Spacing::small, 0);
        auto *title = new QLabel(QStringLiteral("QPMU"), toolbar);
        title->setStyleSheet(QStringLiteral("color: %1; font-size: %2px; font-weight: bold;")
                                     .arg(Theme::Colors::textPrimary.name())
                                     .arg(Theme::Font::large));
        title->setAlignment(Qt::AlignCenter);
        hbox->addStretch();
        hbox->addWidget(title);
        hbox->addStretch();
        hbox->addWidget(m_menuButton);
    }
    layout->addWidget(toolbar);

    setupCentralArea();
    layout->addWidget(m_splitter, 1);

    setupStatusBar();
    layout->addWidget(m_statusBar);

    setCentralWidget(central);

    // Context panel: absolute-positioned overlay child of central widget
    m_contextPanel = new ContextMenuPanel(m_model, central);

    // Repaint visible plot on data changes
    connect(m_model, &QAbstractItemModel::dataChanged, this, [this]() {
        if (m_plotStack->currentWidget())
            m_plotStack->currentWidget()->update();
    });

    // Repaint plots on selection changes
    connect(m_model->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
        m_phasorPlot->update();
        m_waveformPlot->update();
    });

    // Pause state
    connect(m_model, &SignalDataModel::pauseStateChanged, this,
            [this]() { updateStatusIndicator(); });
}

// ---------------------------------------------------------------------------
// Toolbar
// ---------------------------------------------------------------------------

void MainWindow::setupToolbar()
{
    m_menuButton = new QPushButton(QStringLiteral("⋮"));
    m_menuButton->setFixedSize(Theme::Sizing::medium, Theme::Sizing::medium);
    m_menuButton->setCursor(Qt::PointingHandCursor);
    m_menuButton->setStyleSheet(
            QStringLiteral("QPushButton { background: %1; color: %2; border: 1px solid %3;"
                           " border-radius: %4px; font-size: %5px; font-weight: bold; }"
                           "QPushButton:hover { background: %6; }")
                    .arg(Theme::Colors::surfaceElevated.name(), Theme::Colors::textPrimary.name(),
                         Theme::Colors::borderEmphasized.name())
                    .arg(Theme::Radius::large)
                    .arg(Theme::Font::huge)
                    .arg(Theme::Colors::surfaceHover.name()));
    connect(m_menuButton, &QPushButton::clicked, this, [this]() { m_contextPanel->toggle(); });
}

// ---------------------------------------------------------------------------
// Central area: graph tabs + table
// ---------------------------------------------------------------------------

void MainWindow::setupCentralArea()
{
    m_phasorPlot = new PhasorPlot(m_model);
    m_waveformPlot = new WaveformPlot(m_model);

    // Tab bar
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
                    "QTabBar::tab:selected { background: %6; color: %7; border-bottom: 2px solid "
                    "%8; }")
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

    // Table
    m_tableView = new QTableView;
    m_tableView->setModel(m_model);
    m_tableView->setSelectionModel(m_model->selectionModel());
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::MultiSelection);
    m_tableView->setItemDelegate(new SignalTableDelegate(m_tableView));
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->verticalHeader()->setDefaultSectionSize(Theme::Sizing::small
                                                         + Theme::Spacing::small);
    m_tableView->setShowGrid(false);
    m_tableView->setStyleSheet(
            QStringLiteral("QTableView { background: %1; color: %2; gridline-color: %3;"
                           " font-family: '%4'; font-size: %5px; border: none; }"
                           "QHeaderView::section { background: %6; color: %7; padding: 6px;"
                           " border: none; border-bottom: 1px solid %8; font-weight: 600; "
                           "font-size: %9px; }")
                    .arg(Theme::Colors::surface.name(), Theme::Colors::textPrimary.name(),
                         Theme::Colors::borderSubtle.name(), Theme::Font::monospace)
                    .arg(Theme::Font::normal)
                    .arg(Theme::Colors::surfaceElevated.name(), Theme::Colors::textSecondary.name(),
                         Theme::Colors::borderEmphasized.name())
                    .arg(Theme::Font::small));

    // Splitter
    m_splitter = new QSplitter(Qt::Horizontal);
    m_splitter->addWidget(graphPane);
    m_splitter->addWidget(m_tableView);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setHandleWidth(6);
    m_splitter->setStyleSheet(QStringLiteral("QSplitter::handle { background: %1; }"
                                             "QSplitter::handle:hover { background: %2; }")
                                      .arg(Theme::Colors::surfaceElevated.name(),
                                           Theme::Colors::borderEmphasized.name()));
}

// ---------------------------------------------------------------------------
// Status bar
// ---------------------------------------------------------------------------

static QLabel *makeMetricHeader(const QString &text)
{
    auto *label = new QLabel(text);
    label->setStyleSheet(QStringLiteral("color: %1; font-size: %2px; font-weight: bold;")
                                 .arg(Theme::Colors::textTertiary.name())
                                 .arg(Theme::Font::tiny));
    return label;
}

static QLabel *makeMetricValue(const QColor &color)
{
    auto *label = new QLabel(QStringLiteral("--"));
    label->setStyleSheet(
            QStringLiteral("color: %1; font-size: %2px; font-weight: 600; font-family: '%3';")
                    .arg(color.name())
                    .arg(Theme::Font::normal)
                    .arg(Theme::Font::monospace));
    return label;
}

static QFrame *makeSeparator()
{
    auto *sep = new QFrame;
    sep->setFrameShape(QFrame::VLine);
    sep->setFixedHeight(40);
    sep->setStyleSheet(QStringLiteral("color: %1;").arg(Theme::Colors::border.name()));
    return sep;
}

void MainWindow::setupStatusBar()
{
    m_statusBar = new QWidget;
    m_statusBar->setFixedHeight(Theme::Sizing::statusBarHeight);

    auto *hbox = new QHBoxLayout(m_statusBar);
    hbox->setContentsMargins(Theme::Spacing::medium, 0, Theme::Spacing::medium, 0);
    hbox->setSpacing(Theme::Spacing::large);

    // LIVE/PAUSED pill
    m_statusPill = new QWidget;
    m_statusPill->setFixedSize(110, Theme::Sizing::medium);

    auto *pillLayout = new QHBoxLayout(m_statusPill);
    pillLayout->setContentsMargins(Theme::Spacing::small, 0, Theme::Spacing::small, 0);
    pillLayout->setSpacing(Theme::Spacing::small);

    m_statusDot = new QLabel;
    m_statusDot->setFixedSize(8, 8);

    m_statusText = new QLabel;

    pillLayout->addWidget(m_statusDot);
    pillLayout->addWidget(m_statusText);

    hbox->addWidget(m_statusPill);
    hbox->addStretch();

    // Metric columns
    auto addMetricColumn = [&](const QString &header, QLabel *&valueOut, const QColor &color) {
        auto *col = new QVBoxLayout;
        col->setSpacing(Theme::Spacing::tiny);
        col->addWidget(makeMetricHeader(header));
        valueOut = makeMetricValue(color);
        col->addWidget(valueOut);
        hbox->addLayout(col);
    };

    addMetricColumn(QStringLiteral("TIME"), m_timeValue, Theme::Colors::textPrimary);
    hbox->addWidget(makeSeparator());
    addMetricColumn(QStringLiteral("SAMPLING RATE"), m_rateValue, Theme::Colors::info);
    m_rateValue->setText(QStringLiteral("1000.0 Hz"));
    hbox->addWidget(makeSeparator());
    addMetricColumn(QStringLiteral("FREQUENCY"), m_freqValue, Theme::Colors::primary);

    updateStatusIndicator();
}

void MainWindow::updateStatusIndicator()
{
    bool paused = m_model->isPaused();
    QColor accent = paused ? Theme::Colors::error : Theme::Colors::primary;
    QString label = paused ? QStringLiteral("PAUSED") : QStringLiteral("LIVE");

    m_statusText->setText(label);
    m_statusText->setStyleSheet(
            QStringLiteral("color: %1; font-size: %2px; font-weight: bold; font-family: '%3';")
                    .arg(accent.name())
                    .arg(Theme::Font::normal)
                    .arg(Theme::Font::monospace));

    m_statusDot->setStyleSheet(
            QStringLiteral("background: %1; border-radius: 4px;").arg(accent.name()));

    m_statusPill->setStyleSheet(
            QStringLiteral("background: %1; border: %2px solid %3; border-radius: %4px;")
                    .arg(Theme::withAlpha(accent, 32).name(QColor::HexArgb))
                    .arg(Theme::Border::medium)
                    .arg(accent.name())
                    .arg(Theme::Radius::large));

    // Top border on status bar
    m_statusBar->setStyleSheet(QStringLiteral("background: %1; border-top: %2px solid %3;")
                                       .arg(Theme::Colors::surface.name())
                                       .arg(Theme::Border::thick)
                                       .arg(accent.name()));
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (auto *cw = centralWidget())
        m_contextPanel->updateGeometry(cw->width(), cw->height(), Theme::Sizing::toolbarHeight);
}

// ---------------------------------------------------------------------------
// Public setters (called from main.cpp timer callbacks)
// ---------------------------------------------------------------------------

void MainWindow::setLiveMode(bool live)
{
    m_liveMode = live;
    updateStatusIndicator();
}

void MainWindow::setLastSampleTime(const QString &time)
{
    m_timeValue->setText(time);
}

void MainWindow::setSystemFrequency(qreal freq)
{
    m_freqValue->setText(QString::number(freq, 'f', 3) + QStringLiteral(" Hz"));
}
