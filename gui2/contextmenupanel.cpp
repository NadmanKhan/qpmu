#include "contextmenupanel.h"
#include "contextitemmodel.h"
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
#include <QSignalBlocker>

// ---------------------------------------------------------------------------
// Style helpers
// ---------------------------------------------------------------------------

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
// ContextMenuPanel
// ---------------------------------------------------------------------------

ContextMenuPanel::ContextMenuPanel(QWidget *parent)
    : QWidget(parent)
{
    setVisible(false);

    auto *panelLayout = new QVBoxLayout(this);
    panelLayout->setContentsMargins(0, 0, 0, 0);

    m_scrollArea = new QScrollArea;
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setStyleSheet(scrollAreaStyle());
    panelLayout->addWidget(m_scrollArea);

    m_animation = new QPropertyAnimation(this, "geometry", this);
    m_animation->setDuration(ANIM_DURATION);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);

    if (parent)
        parent->installEventFilter(this);
}

void ContextMenuPanel::setModel(ContextItemModel *model)
{
    if (m_model == model)
        return;
    if (m_model)
        disconnect(m_model, nullptr, this, nullptr);
    m_model = model;
    rebuildContent();
    if (m_model) {
        connect(m_model, &QAbstractItemModel::dataChanged,
                this, &ContextMenuPanel::syncFromModel);
    }
}

// ---------------------------------------------------------------------------
// Overlay behavior (unchanged)
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
    if (isVisible() || !m_model)
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
// Build UI from model
// ---------------------------------------------------------------------------

void ContextMenuPanel::rebuildContent()
{
    m_controlMap.clear();

    delete m_contentWidget;
    m_contentWidget = nullptr;

    if (!m_model) {
        m_scrollArea->setWidget(nullptr);
        return;
    }

    m_contentWidget = new QWidget;
    m_contentWidget->setStyleSheet(QStringLiteral("background: transparent;"));

    auto *root = new QVBoxLayout(m_contentWidget);
    root->setContentsMargins(Theme::Spacing::medium, Theme::Spacing::medium,
                             Theme::Spacing::medium, Theme::Spacing::medium);
    root->setSpacing(Theme::Spacing::medium);

    auto *header = new QLabel(QStringLiteral("Controls"));
    header->setStyleSheet(QStringLiteral(
        "color: %1; font-size: %2px; font-weight: bold;")
        .arg(Theme::Colors::textPrimary.name()).arg(Theme::Font::large));
    root->addWidget(header);

    int sectionCount = m_model->rowCount();
    for (int s = 0; s < sectionCount; ++s) {
        QModelIndex sectionIdx = m_model->index(s, 0);
        QString sectionLabel = m_model->data(sectionIdx, ContextItemModel::LabelRole).toString();

        if (s > 0) {
            auto *sep = new QWidget;
            sep->setFixedHeight(1);
            sep->setStyleSheet(QStringLiteral("background: %1;")
                                   .arg(Theme::Colors::borderEmphasized.name()));
            root->addSpacing(Theme::Spacing::small);
            root->addWidget(sep);
            root->addSpacing(Theme::Spacing::small);
        }

        root->addWidget(makeSectionHeader(sectionLabel));

        int itemCount = m_model->rowCount(sectionIdx);
        for (int i = 0; i < itemCount; ++i) {
            QModelIndex itemIdx = m_model->index(i, 0, sectionIdx);
            QString type = m_model->data(itemIdx, ContextItemModel::TypeRole).toString();
            QString label = m_model->data(itemIdx, ContextItemModel::LabelRole).toString();
            int itemId = int(itemIdx.internalId());

            root->addWidget(makeFieldLabel(label));

            if (type == QLatin1String("toggle")) {
                auto *btn = new QPushButton;
                btn->setCursor(Qt::PointingHandCursor);
                btn->setFixedHeight(Theme::Sizing::small);

                QStringList options = m_model->data(itemIdx, ContextItemModel::OptionsRole).toStringList();
                bool val = m_model->data(itemIdx, ContextItemModel::ValueRole).toBool();
                btn->setText(val ? options.value(1) : options.value(0));
                btn->setStyleSheet(toggleButtonStyle(true));

                connect(btn, &QPushButton::clicked, this, [this, itemIdx]() {
                    bool cur = m_model->data(itemIdx, ContextItemModel::ValueRole).toBool();
                    m_model->setValue(itemIdx, !cur);
                });

                m_controlMap[itemId] = btn;
                root->addWidget(btn);

            } else if (type == QLatin1String("dropdown")) {
                auto *combo = new QComboBox;
                combo->addItems(m_model->data(itemIdx, ContextItemModel::OptionsRole).toStringList());
                combo->setFixedHeight(Theme::Sizing::small);
                combo->setStyleSheet(comboBoxStyle());
                combo->setCurrentIndex(m_model->data(itemIdx, ContextItemModel::ValueRole).toInt());

                connect(combo, &QComboBox::currentIndexChanged, this, [this, itemIdx](int idx) {
                    m_model->setValue(itemIdx, idx);
                });

                m_controlMap[itemId] = combo;
                root->addWidget(combo);

            } else if (type == QLatin1String("slider")) {
                qreal min = m_model->data(itemIdx, ContextItemModel::MinRole).toReal();
                qreal max = m_model->data(itemIdx, ContextItemModel::MaxRole).toReal();
                qreal step = m_model->data(itemIdx, ContextItemModel::StepRole).toReal();
                int decimals = m_model->data(itemIdx, ContextItemModel::DecimalsRole).toInt();
                QString unit = m_model->data(itemIdx, ContextItemModel::UnitRole).toString();
                qreal val = m_model->data(itemIdx, ContextItemModel::ValueRole).toReal();

                int scale = 1;
                for (int d = 0; d < decimals; ++d)
                    scale *= 10;

                auto *row = new QWidget;
                auto *rowLayout = new QHBoxLayout(row);
                rowLayout->setContentsMargins(0, 0, 0, 0);
                rowLayout->setSpacing(Theme::Spacing::small);

                auto *slider = new QSlider(Qt::Horizontal);
                slider->setRange(int(min * scale), int(max * scale));
                slider->setSingleStep(int(step * scale));
                slider->setValue(int(val * scale));
                slider->setStyleSheet(sliderStyle());

                auto *valueLabel = new QLabel;
                valueLabel->setFixedWidth(60);
                valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
                valueLabel->setStyleSheet(QStringLiteral(
                    "color: %1; font-size: %2px; font-family: '%3'; font-weight: 600;")
                    .arg(Theme::Colors::textPrimary.name())
                    .arg(Theme::Font::medium)
                    .arg(Theme::Font::monospace));
                valueLabel->setText(QString::number(val, 'f', decimals) + QStringLiteral(" ") + unit);

                connect(slider, &QSlider::valueChanged, this,
                        [this, itemIdx, scale, decimals, unit, valueLabel](int v) {
                    qreal realVal = qreal(v) / scale;
                    m_model->setValue(itemIdx, realVal);
                    valueLabel->setText(
                        QString::number(realVal, 'f', decimals) + QStringLiteral(" ") + unit);
                });

                rowLayout->addWidget(slider, 1);
                rowLayout->addWidget(valueLabel);
                m_controlMap[itemId] = slider;
                root->addWidget(row);

            } else if (type == QLatin1String("button")) {
                auto *btn = new QPushButton;
                btn->setCursor(Qt::PointingHandCursor);
                btn->setFixedHeight(Theme::Sizing::small);

                QVariant val = m_model->data(itemIdx, ContextItemModel::ValueRole);
                QString icon = m_model->data(itemIdx, ContextItemModel::IconRole).toString();
                btn->setText(icon + QStringLiteral("  ") + label);
                btn->setStyleSheet(toggleButtonStyle(val.toBool()));

                connect(btn, &QPushButton::clicked, this, [this, itemIdx]() {
                    QVariant cur = m_model->data(itemIdx, ContextItemModel::ValueRole);
                    m_model->setValue(itemIdx, !cur.toBool());
                });

                m_controlMap[itemId] = btn;
                root->addWidget(btn);
            }
        }
    }

    root->addStretch();
    m_scrollArea->setWidget(m_contentWidget);
}

// ---------------------------------------------------------------------------
// Sync controls when model values change
// ---------------------------------------------------------------------------

void ContextMenuPanel::syncFromModel(const QModelIndex &topLeft, const QModelIndex &,
                                     const QList<int> &roles)
{
    if (!roles.contains(ContextItemModel::ValueRole))
        return;

    int itemId = int(topLeft.internalId());
    auto *widget = m_controlMap.value(itemId);
    if (!widget)
        return;

    QString type = m_model->data(topLeft, ContextItemModel::TypeRole).toString();
    QVariant value = m_model->data(topLeft, ContextItemModel::ValueRole);

    if (type == QLatin1String("toggle")) {
        auto *btn = qobject_cast<QPushButton *>(widget);
        QStringList options = m_model->data(topLeft, ContextItemModel::OptionsRole).toStringList();
        btn->setText(value.toBool() ? options.value(1) : options.value(0));

    } else if (type == QLatin1String("dropdown")) {
        auto *combo = qobject_cast<QComboBox *>(widget);
        QSignalBlocker blocker(combo);
        combo->setCurrentIndex(value.toInt());

    } else if (type == QLatin1String("slider")) {
        auto *slider = qobject_cast<QSlider *>(widget);
        int decimals = m_model->data(topLeft, ContextItemModel::DecimalsRole).toInt();
        int scale = 1;
        for (int d = 0; d < decimals; ++d)
            scale *= 10;
        QSignalBlocker blocker(slider);
        slider->setValue(int(value.toReal() * scale));

    } else if (type == QLatin1String("button")) {
        auto *btn = qobject_cast<QPushButton *>(widget);
        QString icon = m_model->data(topLeft, ContextItemModel::IconRole).toString();
        QString label = m_model->data(topLeft, ContextItemModel::LabelRole).toString();
        btn->setText(icon + QStringLiteral("  ") + label);
        btn->setStyleSheet(toggleButtonStyle(value.toBool()));
    }
}
