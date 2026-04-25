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
#include <QFrame>
#include <QResizeEvent>

// ── Style helpers ───────────────────────────────────────────────────────────

static QString toolbarButtonStyle()
{
    return QStringLiteral(
        "QPushButton { background: %1; color: %2; border: 1px solid %3;"
        " border-radius: %4px; font-size: %5px; font-weight: bold; }"
        "QPushButton:hover { background: %6; }")
        .arg(Theme::Colors::surfaceElevated.name(), Theme::Colors::textPrimary.name(),
             Theme::Colors::borderEmphasized.name())
        .arg(Theme::Radius::large)
        .arg(Theme::Font::huge)
        .arg(Theme::Colors::surfaceHover.name());
}

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
    m_titleLabel->setStyleSheet(
        QStringLiteral("color: %1; font-size: %2px; font-weight: bold;")
            .arg(Theme::Colors::textPrimary.name()).arg(Theme::Font::large));
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
    connect(m_menuButton, &QPushButton::clicked, this, [this]() {
        m_contextPanel->toggle();
    });

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
    statusLayout->setSpacing(Theme::Spacing::large);

    m_statusPill = new QWidget;
    m_statusPill->setFixedSize(110, Theme::Sizing::medium);
    {
        auto *pillLayout = new QHBoxLayout(m_statusPill);
        pillLayout->setContentsMargins(Theme::Spacing::small, 0, Theme::Spacing::small, 0);
        pillLayout->setSpacing(Theme::Spacing::small);

        m_statusDot = new QLabel;
        m_statusDot->setFixedSize(8, 8);
        m_statusText = new QLabel;

        pillLayout->addWidget(m_statusDot);
        pillLayout->addWidget(m_statusText);
    }

    statusLayout->addWidget(m_statusPill);
    statusLayout->addStretch();

    auto addMetricColumn = [&](const QString &header, QLabel *&valueOut, const QColor &color) {
        auto *col = new QVBoxLayout;
        col->setSpacing(Theme::Spacing::tiny);
        col->addWidget(makeMetricHeader(header));
        valueOut = makeMetricValue(color);
        col->addWidget(valueOut);
        statusLayout->addLayout(col);
    };

    addMetricColumn(QStringLiteral("TIME"), m_timeValue, Theme::Colors::textPrimary);
    statusLayout->addWidget(makeSeparator());
    addMetricColumn(QStringLiteral("SAMPLING RATE"), m_rateValue, Theme::Colors::info);
    m_rateValue->setText(QStringLiteral("1000.0 Hz"));
    statusLayout->addWidget(makeSeparator());
    addMetricColumn(QStringLiteral("FREQUENCY"), m_freqValue, Theme::Colors::primary);

    layout->addWidget(m_statusBar);
    setCentralWidget(central);
    updateStatusIndicator();

    // -- Context panel (overlay) --
    m_contextPanel = new ContextMenuPanel(central);

    // -- Initial screen --
    auto *liveScreen = new LiveMonitorScreen(m_model);
    pushScreen(liveScreen);

    connect(m_model, &SignalDataModel::pauseStateChanged, this,
            [this]() { updateStatusIndicator(); });
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

    m_statusBar->setStyleSheet(
        QStringLiteral("background: %1; border-top: %2px solid %3;")
            .arg(Theme::Colors::surface.name())
            .arg(Theme::Border::thick)
            .arg(accent.name()));
}

// ── Public setters ──────────────────────────────────────────────────────────

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
