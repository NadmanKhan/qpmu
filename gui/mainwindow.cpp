#include "mainwindow.h"
#include "signaldatamodel.h"
#include "screen.h"
#include "livemonitorscreen.h"
#include "contextmenupanel.h"
#include "theme.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QTimer>
#include <QDateTime>

// ── Style helpers ───────────────────────────────────────────────────────────

static QString toolbarButtonStyle()
{
    return QStringLiteral("QPushButton { background: %1; color: %2; border: 1px solid %3;"
                          " border-radius: %4px; font-size: %5px; font-weight: bold; }"
                          "QPushButton:hover { background: %6; }")
            .arg(Theme::Colors::surfaceElevated.name(), Theme::Colors::textPrimary.name(),
                 Theme::Colors::borderEmphasized.name())
            .arg(Theme::Radius::large)
            .arg(Theme::Font::huge)
            .arg(Theme::Colors::surfaceHover.name());
}

static QLabel *makeMetricLabel(const QString &text)
{
    auto *label = new QLabel(text);
    label->setStyleSheet(QStringLiteral("color: %1; font-size: %2px;")
                                 .arg(Theme::Colors::textTertiary.name())
                                 .arg(Theme::Font::small));
    return label;
}

static QLabel *makeMetricValue(const QColor &color)
{
    auto *label = new QLabel(QStringLiteral("--"));
    label->setStyleSheet(
            QStringLiteral("color: %1; font-size: %2px; font-weight: 600; font-family: '%3';")
                    .arg(color.name())
                    .arg(Theme::Font::medium)
                    .arg(Theme::Font::monospace));
    return label;
}

static QLabel *makeDotSeparator()
{
    auto *dot = new QLabel(QStringLiteral("·"));
    dot->setStyleSheet(QStringLiteral("color: %1; font-size: %2px;")
                               .arg(Theme::Colors::border.name())
                               .arg(Theme::Font::large));
    return dot;
}

// ── MainWindow ──────────────────────────────────────────────────────────────

MainWindow::MainWindow(SignalDataModel *model, QWidget *parent)
    : QMainWindow(parent), m_model(model)
{
    setWindowTitle(QStringLiteral("QPMU - Phasor Measurement Unit"));
    resize(1280, 800);

    auto *central = new QWidget;
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // -- Toolbar --
    m_titleLabel = new QLabel;
    m_titleLabel->setStyleSheet(QStringLiteral("color: %1; font-size: %2px; font-weight: bold;")
                                        .arg(Theme::Colors::textPrimary.name())
                                        .arg(Theme::Font::large));
    m_titleLabel->setAlignment(Qt::AlignCenter);

    m_backButton = new QPushButton(QStringLiteral("‹"));
    m_backButton->setFixedSize(Theme::Sizing::medium, Theme::Sizing::medium);
    m_backButton->setCursor(Qt::PointingHandCursor);
    m_backButton->setStyleSheet(toolbarButtonStyle());
    connect(m_backButton, &QPushButton::clicked, this, &MainWindow::popScreen);

    m_menuButton = new QPushButton(QStringLiteral("⋮"));
    m_menuButton->setFixedSize(Theme::Sizing::medium, Theme::Sizing::medium);
    m_menuButton->setCursor(Qt::PointingHandCursor);
    m_menuButton->setStyleSheet(toolbarButtonStyle());
    connect(m_menuButton, &QPushButton::clicked, this, [this]() { m_contextPanel->toggle(); });

    auto *toolbar = new QWidget(central);
    toolbar->setObjectName(QStringLiteral("toolbar"));
    toolbar->setFixedHeight(Theme::Sizing::toolbarHeight);
    toolbar->setStyleSheet(
            QStringLiteral("QWidget#toolbar { background: %1; border-bottom: 1px solid %2; }")
                    .arg(Theme::Colors::surface.name(), Theme::Colors::borderEmphasized.name()));
    {
        auto *hbox = new QHBoxLayout(toolbar);
        hbox->setContentsMargins(Theme::Spacing::small, 0, Theme::Spacing::small, 0);
        hbox->addWidget(m_backButton);
        hbox->addStretch();
        hbox->addWidget(m_titleLabel);
        hbox->addStretch();
        hbox->addWidget(m_menuButton);
    }
    layout->addWidget(toolbar);

    // -- Screen stack --
    m_screenStack = new QStackedWidget;
    layout->addWidget(m_screenStack, 1);

    // -- Status bar --
    m_statusBar = new QWidget;
    m_statusBar->setFixedHeight(Theme::Sizing::statusBarHeight);

    auto *statusLayout = new QHBoxLayout(m_statusBar);
    statusLayout->setContentsMargins(Theme::Spacing::medium, 0, Theme::Spacing::medium, 0);
    statusLayout->setSpacing(0);

    auto *connectionGroup = new QWidget;
    {
        auto *vbox = new QVBoxLayout(connectionGroup);
        vbox->setContentsMargins(0, 0, 0, 0);
        vbox->setSpacing(0);

        // Row 1: dot + label
        auto *row1 = new QHBoxLayout;
        row1->setSpacing(Theme::Spacing::tiny);
        m_connectionDot = new QLabel;
        m_connectionDot->setFixedSize(7, 7);
        m_connectionLabel = new QLabel;
        row1->addWidget(m_connectionDot);
        row1->addWidget(m_connectionLabel);
        row1->addStretch();
        vbox->addLayout(row1);

        // Row 2: sample time, freq, rate (hidden when offline)
        m_liveMetrics = new QWidget;
        {
            auto *row2 = new QHBoxLayout(m_liveMetrics);
            row2->setContentsMargins(0, 0, 0, 0);
            row2->setSpacing(Theme::Spacing::small);

            row2->addWidget(makeMetricLabel(QStringLiteral("Last")));
            m_sampleTimeValue = makeMetricValue(Theme::Colors::textSecondary);
            row2->addWidget(m_sampleTimeValue);

            row2->addWidget(makeDotSeparator());

            row2->addWidget(makeMetricLabel(QStringLiteral("Freq")));
            m_freqValue = makeMetricValue(Theme::Colors::primary);
            row2->addWidget(m_freqValue);

            row2->addWidget(makeDotSeparator());

            row2->addWidget(makeMetricLabel(QStringLiteral("Rate")));
            m_rateValue = makeMetricValue(Theme::Colors::info);
            m_rateValue->setText(QStringLiteral("1000.0 Hz"));
            row2->addWidget(m_rateValue);
        }
        vbox->addWidget(m_liveMetrics);
    }
    statusLayout->addWidget(connectionGroup);

    statusLayout->addStretch();

    m_utcLabel = new QLabel;
    m_utcLabel->setStyleSheet(
            QStringLiteral("color: %1; font-size: %2px; font-family: '%3';")
                    .arg(Theme::Colors::textSecondary.name())
                    .arg(Theme::Font::medium)
                    .arg(Theme::Font::monospace));
    statusLayout->addWidget(m_utcLabel);

    layout->addWidget(m_statusBar);
    setCentralWidget(central);
    updateConnectionIndicator();

    // -- UTC clock --
    auto updateUtc = [this]() {
        m_utcLabel->setText(
                QDateTime::currentDateTimeUtc().toString(QStringLiteral("hh:mm:ss · ddd, MMM d")));
    };
    updateUtc();
    auto *utcTimer = new QTimer(this);
    connect(utcTimer, &QTimer::timeout, this, updateUtc);
    utcTimer->start(1000);

    // -- Context panel (overlay) --
    m_contextPanel = new ContextMenuPanel(central);

    // -- Initial screen --
    auto *liveScreen = new LiveMonitorScreen(m_model);
    pushScreen(liveScreen);
}

// ── Screen navigation ───────────────────────────────────────────────────────

void MainWindow::pushScreen(Screen *screen)
{
    m_screenStack->addWidget(screen);
    m_screenStack->setCurrentWidget(screen);
    connect(screen, &Screen::navigateTo, this, &MainWindow::pushScreen);
    updateToolbar();
}

void MainWindow::popScreen()
{
    if (m_screenStack->count() <= 1)
        return;
    auto *screen = m_screenStack->currentWidget();
    m_screenStack->removeWidget(screen);
    screen->deleteLater();
    updateToolbar();
}

void MainWindow::updateToolbar()
{
    auto *screen = qobject_cast<Screen *>(m_screenStack->currentWidget());
    if (!screen)
        return;

    m_titleLabel->setText(screen->title());
    m_backButton->setVisible(m_screenStack->count() > 1);
    m_menuButton->setVisible(screen->contextModel() != nullptr);

    m_contextPanel->hidePanel();
    m_contextPanel->setModel(screen->contextModel());
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (auto *cw = centralWidget())
        m_contextPanel->updateGeometry(cw->width(), cw->height(), Theme::Sizing::toolbarHeight);
}

// ── Status bar ──────────────────────────────────────────────────────────────

void MainWindow::updateConnectionIndicator()
{
    QColor accent = m_liveMode ? Theme::Colors::primary : Theme::Colors::textTertiary;
    QString label = m_liveMode ? QStringLiteral("Source") : QStringLiteral("No source");

    m_connectionDot->setStyleSheet(
            QStringLiteral("background: %1; border-radius: 3px;").arg(accent.name()));

    m_connectionLabel->setText(label);
    m_connectionLabel->setStyleSheet(
            QStringLiteral("color: %1; font-size: %2px; font-weight: bold;")
                    .arg(accent.name())
                    .arg(Theme::Font::small));

    m_sampleTimeValue->setText(QStringLiteral("--"));
    m_liveMetrics->setVisible(m_liveMode);

    m_statusBar->setStyleSheet(
            QStringLiteral("background: %1;").arg(Theme::Colors::surface.name()));
}

// ── Public setters ──────────────────────────────────────────────────────────

void MainWindow::setLiveMode(bool live)
{
    m_liveMode = live;
    updateConnectionIndicator();
}

void MainWindow::setLastSampleTime(const QString &time)
{
    m_sampleTimeValue->setText(time);
}

void MainWindow::setSystemFrequency(qreal freq)
{
    m_freqValue->setText(QString::number(freq, 'f', 3) + QStringLiteral(" Hz"));
}
