#include "contextmenupanel.h"
#include "signaldatamodel.h"
#include "theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSlider>
#include <QScrollArea>
#include <QPainter>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QMouseEvent>

static QLabel *makeSectionHeader(const QString &text)
{
    auto *label = new QLabel(text);
    label->setStyleSheet(QStringLiteral(
        "color: %1; font-size: %2px; font-weight: bold; letter-spacing: 1px;")
        .arg(Theme::Colors::textTertiary.name()).arg(Theme::Font::tiny));
    return label;
}

static QLabel *makeFieldLabel(const QString &text)
{
    auto *label = new QLabel(text);
    label->setStyleSheet(QStringLiteral(
        "color: %1; font-size: %2px;")
        .arg(Theme::Colors::textSecondary.name()).arg(Theme::Font::medium));
    return label;
}

static QString toggleButtonStyle(bool active, const QColor &accent = Theme::Colors::primary)
{
    QColor bg = active ? Theme::withAlpha(accent, 40) : Theme::Colors::surfaceElevated;
    QColor border = active ? accent : Theme::Colors::borderEmphasized;
    QColor text = active ? accent : Theme::Colors::textSecondary;
    return QStringLiteral(
        "QPushButton { background: %1; color: %2; border: 1px solid %3;"
        " border-radius: %4px; font-size: %5px; font-weight: 600;"
        " padding: 6px 14px; font-family: '%6'; }"
        "QPushButton:hover { background: %7; }")
        .arg(bg.name(QColor::HexArgb), text.name(), border.name())
        .arg(Theme::Radius::small)
        .arg(Theme::Font::medium)
        .arg(Theme::Font::monospace,
             Theme::Colors::surfaceHover.name());
}

static QString comboBoxStyle()
{
    return QStringLiteral(
        "QComboBox { background: %1; color: %2; border: 1px solid %3;"
        " border-radius: %4px; padding: 6px 10px; font-size: %5px;"
        " font-family: '%6'; font-weight: 600; }"
        "QComboBox::drop-down { border: none; width: 24px; }"
        "QComboBox::down-arrow { image: none; border-left: 4px solid transparent;"
        " border-right: 4px solid transparent; border-top: 5px solid %7;"
        " margin-right: 8px; }"
        "QComboBox QAbstractItemView { background: %8; color: %9;"
        " selection-background-color: %10; border: 1px solid %11;"
        " font-size: %12px; }")
        .arg(Theme::Colors::surfaceElevated.name(),
             Theme::Colors::textPrimary.name(),
             Theme::Colors::borderEmphasized.name())
        .arg(Theme::Radius::small)
        .arg(Theme::Font::medium)
        .arg(Theme::Font::monospace,
             Theme::Colors::textSecondary.name(),
             Theme::Colors::surface.name(),
             Theme::Colors::textPrimary.name(),
             Theme::withAlpha(Theme::Colors::primary, 40).name(QColor::HexArgb),
             Theme::Colors::borderEmphasized.name())
        .arg(Theme::Font::medium);
}

static QString sliderStyle()
{
    return QStringLiteral(
        "QSlider::groove:horizontal { background: %1; height: 4px; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: %2; width: 16px; height: 16px;"
        " margin: -6px 0; border-radius: 8px; border: 2px solid %3; }"
        "QSlider::sub-page:horizontal { background: %4; border-radius: 2px; }")
        .arg(Theme::Colors::borderEmphasized.name(),
             Theme::Colors::primary.name(),
             Theme::Colors::surface.name(),
             Theme::Colors::primary.name());
}

static QString scrollAreaStyle()
{
    return QStringLiteral(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical { background: %1; width: 6px; border-radius: 3px; }"
        "QScrollBar::handle:vertical { background: %2; border-radius: 3px; min-height: 30px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }")
        .arg(Theme::Colors::surface.name(),
             Theme::Colors::borderEmphasized.name());
}

// ---------------------------------------------------------------------------

ContextMenuPanel::ContextMenuPanel(SignalDataModel *model, QWidget *parent)
    : QWidget(parent), m_model(model)
{
    setVisible(false);

    // Scroll area fills the panel
    auto *panelLayout = new QVBoxLayout(this);
    panelLayout->setContentsMargins(0, 0, 0, 0);

    auto *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet(scrollAreaStyle());
    scrollArea->setWidget(buildContent());
    panelLayout->addWidget(scrollArea);

    // Animation
    m_animation = new QPropertyAnimation(this, "geometry", this);
    m_animation->setDuration(ANIM_DURATION);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);

    // Event filter on parent for click-outside and resize tracking
    if (parent)
        parent->installEventFilter(this);

    syncFromModel();

    connect(m_model, &SignalDataModel::magnitudeModeChanged, this, &ContextMenuPanel::syncFromModel);
    connect(m_model, &SignalDataModel::phaseReferenceChanged, this, &ContextMenuPanel::syncFromModel);
    connect(m_model, &SignalDataModel::pauseStateChanged, this, &ContextMenuPanel::syncFromModel);
    connect(m_model, &SignalDataModel::voltageScalingChanged, this, &ContextMenuPanel::syncFromModel);
    connect(m_model, &SignalDataModel::currentScalingChanged, this, &ContextMenuPanel::syncFromModel);
}

// ---------------------------------------------------------------------------

void ContextMenuPanel::toggle()
{
    if (isVisible())
        hidePanel();
    else
        showPanel();
}

void ContextMenuPanel::showPanel()
{
    if (isVisible())
        return;

    int h = m_parentHeight - m_topOffset - Theme::Sizing::statusBarHeight;
    QRect hidden(m_parentWidth, m_topOffset, PANEL_WIDTH, h);
    QRect target(m_parentWidth - PANEL_WIDTH, m_topOffset, PANEL_WIDTH, h);

    setGeometry(hidden);
    show();
    raise();

    m_animation->stop();
    m_animation->disconnect();
    m_animation->setStartValue(hidden);
    m_animation->setEndValue(target);
    m_animation->start();
}

void ContextMenuPanel::hidePanel()
{
    if (!isVisible())
        return;

    int h = m_parentHeight - m_topOffset - Theme::Sizing::statusBarHeight;
    QRect hidden(m_parentWidth, m_topOffset, PANEL_WIDTH, h);

    m_animation->stop();
    m_animation->disconnect();
    m_animation->setStartValue(geometry());
    m_animation->setEndValue(hidden);
    connect(m_animation, &QPropertyAnimation::finished, this, &QWidget::hide);
    m_animation->start();
}

void ContextMenuPanel::updateGeometry(int parentWidth, int parentHeight, int topOffset)
{
    m_parentWidth = parentWidth;
    m_parentHeight = parentHeight;
    m_topOffset = topOffset;

    if (isVisible() && m_animation->state() != QPropertyAnimation::Running) {
        int h = parentHeight - topOffset - Theme::Sizing::statusBarHeight;
        setGeometry(parentWidth - PANEL_WIDTH, topOffset, PANEL_WIDTH, h);
    }
}

// ---------------------------------------------------------------------------

bool ContextMenuPanel::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == parentWidget()) {
        if (event->type() == QEvent::MouseButtonPress && isVisible()) {
            auto *me = static_cast<QMouseEvent *>(event);
            QPoint local = mapFromParent(me->position().toPoint());
            if (!rect().contains(local))
                hidePanel();
        }
        if (event->type() == QEvent::Resize) {
            auto *re = static_cast<QResizeEvent *>(event);
            updateGeometry(re->size().width(), re->size().height(), m_topOffset);
        }
    }
    return QWidget::eventFilter(obj, event);
}

void ContextMenuPanel::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), Theme::Colors::surface);
    p.setPen(QPen(Theme::Colors::borderEmphasized, Theme::Border::medium));
    p.drawLine(0, 0, 0, height());
}

// ---------------------------------------------------------------------------

QWidget *ContextMenuPanel::buildContent()
{
    auto *content = new QWidget;
    content->setStyleSheet(QStringLiteral("background: transparent;"));

    auto *root = new QVBoxLayout(content);
    root->setContentsMargins(Theme::Spacing::medium, Theme::Spacing::medium,
                             Theme::Spacing::medium, Theme::Spacing::medium);
    root->setSpacing(Theme::Spacing::medium);

    // ---- Header ----
    auto *header = new QLabel(QStringLiteral("Controls"));
    header->setStyleSheet(QStringLiteral(
        "color: %1; font-size: %2px; font-weight: bold;")
        .arg(Theme::Colors::textPrimary.name()).arg(Theme::Font::large));
    root->addWidget(header);

    // ---- DATA CONTROLS ----
    root->addWidget(makeSectionHeader(QStringLiteral("DATA CONTROLS")));

    // Magnitude mode
    root->addWidget(makeFieldLabel(QStringLiteral("Magnitude")));
    m_magnitudeToggle = new QPushButton;
    m_magnitudeToggle->setCursor(Qt::PointingHandCursor);
    m_magnitudeToggle->setFixedHeight(Theme::Sizing::small);
    root->addWidget(m_magnitudeToggle);
    connect(m_magnitudeToggle, &QPushButton::clicked, this, [this]() {
        auto mode = m_model->magnitudeMode() == SignalDataModel::RMS
                        ? SignalDataModel::Peak
                        : SignalDataModel::RMS;
        m_model->setMagnitudeMode(mode);
    });

    // Phase reference
    root->addWidget(makeFieldLabel(QStringLiteral("Phase Reference")));
    m_phaseRefCombo = new QComboBox;
    m_phaseRefCombo->addItems({
        QStringLiteral("Absolute"),
        QStringLiteral("VA"), QStringLiteral("VB"), QStringLiteral("VC"),
        QStringLiteral("IA"), QStringLiteral("IB"), QStringLiteral("IC"),
    });
    m_phaseRefCombo->setFixedHeight(Theme::Sizing::small);
    m_phaseRefCombo->setStyleSheet(comboBoxStyle());
    root->addWidget(m_phaseRefCombo);
    connect(m_phaseRefCombo, &QComboBox::currentIndexChanged, this, [this](int idx) {
        m_model->setPhaseReferenceIndex(idx - 1);
    });

    // Pause/Resume
    root->addSpacing(Theme::Spacing::small);
    m_pauseButton = new QPushButton;
    m_pauseButton->setCursor(Qt::PointingHandCursor);
    m_pauseButton->setFixedHeight(Theme::Sizing::small);
    root->addWidget(m_pauseButton);
    connect(m_pauseButton, &QPushButton::clicked, this, [this]() {
        m_model->setIsPaused(!m_model->isPaused());
    });

    // ---- Separator ----
    auto *sep = new QWidget;
    sep->setFixedHeight(1);
    sep->setStyleSheet(QStringLiteral("background: %1;").arg(Theme::Colors::borderEmphasized.name()));
    root->addSpacing(Theme::Spacing::small);
    root->addWidget(sep);
    root->addSpacing(Theme::Spacing::small);

    // ---- PLOT CONTROLS ----
    root->addWidget(makeSectionHeader(QStringLiteral("PLOT CONTROLS")));

    // Voltage scaling
    root->addWidget(makeFieldLabel(QStringLiteral("Voltage Scaling")));
    m_voltageScaleToggle = new QPushButton;
    m_voltageScaleToggle->setCursor(Qt::PointingHandCursor);
    m_voltageScaleToggle->setFixedHeight(Theme::Sizing::small);
    root->addWidget(m_voltageScaleToggle);
    connect(m_voltageScaleToggle, &QPushButton::clicked, this, [this]() {
        auto mode = m_model->voltageScalingMode() == SignalDataModel::Dynamic
                        ? SignalDataModel::Manual
                        : SignalDataModel::Dynamic;
        m_model->setVoltageScalingMode(mode);
    });

    m_voltageSliderRow = new QWidget;
    auto *vSliderLayout = new QHBoxLayout(m_voltageSliderRow);
    vSliderLayout->setContentsMargins(0, 0, 0, 0);
    vSliderLayout->setSpacing(Theme::Spacing::small);
    m_voltageSlider = new QSlider(Qt::Horizontal);
    m_voltageSlider->setRange(10, 500);
    m_voltageSlider->setSingleStep(5);
    m_voltageSlider->setStyleSheet(sliderStyle());
    m_voltageSliderValue = new QLabel;
    m_voltageSliderValue->setFixedWidth(60);
    m_voltageSliderValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_voltageSliderValue->setStyleSheet(QStringLiteral(
        "color: %1; font-size: %2px; font-family: '%3'; font-weight: 600;")
        .arg(Theme::Colors::textPrimary.name())
        .arg(Theme::Font::medium)
        .arg(Theme::Font::monospace));
    vSliderLayout->addWidget(m_voltageSlider, 1);
    vSliderLayout->addWidget(m_voltageSliderValue);
    root->addWidget(m_voltageSliderRow);
    connect(m_voltageSlider, &QSlider::valueChanged, this, [this](int val) {
        m_model->setVoltageCutoff(val);
        m_voltageSliderValue->setText(QString::number(val) + QStringLiteral(" V"));
    });

    // Current scaling
    root->addSpacing(Theme::Spacing::small);
    root->addWidget(makeFieldLabel(QStringLiteral("Current Scaling")));
    m_currentScaleToggle = new QPushButton;
    m_currentScaleToggle->setCursor(Qt::PointingHandCursor);
    m_currentScaleToggle->setFixedHeight(Theme::Sizing::small);
    root->addWidget(m_currentScaleToggle);
    connect(m_currentScaleToggle, &QPushButton::clicked, this, [this]() {
        auto mode = m_model->currentScalingMode() == SignalDataModel::Dynamic
                        ? SignalDataModel::Manual
                        : SignalDataModel::Dynamic;
        m_model->setCurrentScalingMode(mode);
    });

    m_currentSliderRow = new QWidget;
    auto *cSliderLayout = new QHBoxLayout(m_currentSliderRow);
    cSliderLayout->setContentsMargins(0, 0, 0, 0);
    cSliderLayout->setSpacing(Theme::Spacing::small);
    m_currentSlider = new QSlider(Qt::Horizontal);
    m_currentSlider->setRange(10, 500);  // 1.0A to 50.0A in tenths
    m_currentSlider->setSingleStep(5);   // 0.5A steps
    m_currentSlider->setStyleSheet(sliderStyle());
    m_currentSliderValue = new QLabel;
    m_currentSliderValue->setFixedWidth(60);
    m_currentSliderValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_currentSliderValue->setStyleSheet(QStringLiteral(
        "color: %1; font-size: %2px; font-family: '%3'; font-weight: 600;")
        .arg(Theme::Colors::textPrimary.name())
        .arg(Theme::Font::medium)
        .arg(Theme::Font::monospace));
    cSliderLayout->addWidget(m_currentSlider, 1);
    cSliderLayout->addWidget(m_currentSliderValue);
    root->addWidget(m_currentSliderRow);
    connect(m_currentSlider, &QSlider::valueChanged, this, [this](int val) {
        m_model->setCurrentCutoff(val / 10.0);
        m_currentSliderValue->setText(QString::number(val / 10.0, 'f', 1) + QStringLiteral(" A"));
    });

    root->addStretch();

    return content;
}

void ContextMenuPanel::syncFromModel()
{
    // Magnitude toggle
    bool isRMS = m_model->magnitudeMode() == SignalDataModel::RMS;
    m_magnitudeToggle->setText(isRMS ? QStringLiteral("RMS") : QStringLiteral("Peak"));
    m_magnitudeToggle->setStyleSheet(toggleButtonStyle(true));

    // Phase reference
    QSignalBlocker blocker(m_phaseRefCombo);
    m_phaseRefCombo->setCurrentIndex(m_model->phaseReferenceIndex() + 1);

    // Pause button
    bool paused = m_model->isPaused();
    m_pauseButton->setText(paused ? QStringLiteral("▶  Resume") : QStringLiteral("⏸  Pause"));
    QColor pauseAccent = paused ? Theme::Colors::primary : Theme::Colors::error;
    m_pauseButton->setStyleSheet(toggleButtonStyle(!paused, pauseAccent));

    // Voltage scaling
    bool vDynamic = m_model->voltageScalingMode() == SignalDataModel::Dynamic;
    m_voltageScaleToggle->setText(vDynamic ? QStringLiteral("Dynamic") : QStringLiteral("Manual"));
    m_voltageScaleToggle->setStyleSheet(toggleButtonStyle(vDynamic));
    m_voltageSliderRow->setVisible(!vDynamic);
    if (!vDynamic) {
        QSignalBlocker sb(m_voltageSlider);
        m_voltageSlider->setValue(qRound(m_model->voltageCutoff()));
        m_voltageSliderValue->setText(QString::number(qRound(m_model->voltageCutoff())) + QStringLiteral(" V"));
    }

    // Current scaling
    bool cDynamic = m_model->currentScalingMode() == SignalDataModel::Dynamic;
    m_currentScaleToggle->setText(cDynamic ? QStringLiteral("Dynamic") : QStringLiteral("Manual"));
    m_currentScaleToggle->setStyleSheet(toggleButtonStyle(cDynamic));
    m_currentSliderRow->setVisible(!cDynamic);
    if (!cDynamic) {
        QSignalBlocker sb(m_currentSlider);
        m_currentSlider->setValue(qRound(m_model->currentCutoff() * 10));
        m_currentSliderValue->setText(QString::number(m_model->currentCutoff(), 'f', 1) + QStringLiteral(" A"));
    }
}
