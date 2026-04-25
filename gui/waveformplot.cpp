#include "waveformplot.h"
#include "signaldatamodel.h"
#include "theme.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QtMath>

WaveformPlot::WaveformPlot(SignalDataModel *model, QWidget *parent)
    : QWidget(parent), m_model(model)
{
    setMinimumSize(200, 200);
}

QRectF WaveformPlot::chartRect() const
{
    qreal w = width() * CHART_WIDTH_RATIO;
    qreal h = height() * CHART_HEIGHT_RATIO;
    return QRectF((width() - w) / 2, (height() - h) / 2, w, h);
}

qreal WaveformPlot::dataToScreenX(qreal t) const
{
    QRectF cr = chartRect();
    return cr.x() + (t / CYCLE_COUNT) * cr.width();
}

qreal WaveformPlot::dataToScreenY(qreal y, bool isVoltage) const
{
    QRectF cr = chartRect();
    qreal maxVal = isVoltage ? m_model->voltageCutoff() : m_model->currentCutoff();
    qreal norm = (y / maxVal) * VERTICAL_SCALE;
    return cr.y() + cr.height() / 2.0 - norm * cr.height();
}

// ── Painting ────────────────────────────────────────────────────────────────

void WaveformPlot::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Theme::Colors::surface);

    drawGrid(p);
    drawWaveforms(p);
    drawAxesLabels(p);
    drawTooltipLabels(p);
}

void WaveformPlot::drawGrid(QPainter &p)
{
    QRectF cr = chartRect();
    qreal maxV = m_model->voltageCutoff();

    // Horizontal grid lines
    for (int i = 0; i < GRID_ROW_COUNT; ++i) {
        qreal val = -maxV + i * (2.0 * maxV / (GRID_ROW_COUNT - 1));
        qreal y = dataToScreenY(val, true);
        bool center = (i == GRID_ROW_COUNT / 2);
        p.setPen(QPen(center ? Theme::Colors::borderEmphasized : Theme::Colors::borderSubtle,
                      center ? Theme::Border::thick : Theme::Border::thin));
        p.drawLine(QPointF(cr.left() - Theme::Spacing::small, y),
                   QPointF(cr.right() + Theme::Spacing::small, y));
    }

    // Vertical grid lines
    int vLines = CYCLE_COUNT * GRID_DIVS_PER_CYCLE + 1;
    for (int i = 0; i < vLines; ++i) {
        qreal t = qreal(i) / GRID_DIVS_PER_CYCLE;
        qreal x = dataToScreenX(t);
        bool major = (i % GRID_DIVS_PER_CYCLE == 0);
        p.setPen(QPen(major ? Theme::Colors::border : Theme::Colors::borderSubtle,
                      major ? Theme::Border::thick : Theme::Border::thin));
        p.drawLine(QPointF(x, cr.top() - Theme::Spacing::small),
                   QPointF(x, cr.bottom() + Theme::Spacing::small));
    }
}

void WaveformPlot::drawWaveforms(QPainter &p)
{
    int count = m_model->rowCount();

    for (int si = 0; si < count; ++si) {
        qreal mag = m_model->data(m_model->index(si, 0), SignalDataModel::MagnitudeRole).toReal();
        qreal phase = m_model->data(m_model->index(si, 0), SignalDataModel::PhaseAngleRole).toReal();
        QColor color = m_model->data(m_model->index(si, 0), Qt::DecorationRole).value<QColor>();
        QString type = m_model->data(m_model->index(si, 0), SignalDataModel::TypeSymbolRole).toString();
        bool isV = (type == "V");
        qreal phaseRad = qDegreesToRadians(phase);

        QPainterPath path;
        for (int i = 0; i <= POINTS_PER_CYCLE * CYCLE_COUNT; ++i) {
            qreal t = qreal(i) / POINTS_PER_CYCLE;
            qreal y = mag * qSin(2.0 * M_PI * t + phaseRad);
            qreal sx = dataToScreenX(t);
            qreal sy = dataToScreenY(y, isV);
            if (i == 0)
                path.moveTo(sx, sy);
            else
                path.lineTo(sx, sy);
        }

        p.setPen(QPen(color, LINE_WIDTH, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);
    }
}

void WaveformPlot::drawAxesLabels(QPainter &p)
{
    QRectF cr = chartRect();
    qreal maxV = m_model->voltageCutoff();
    qreal maxI = m_model->currentCutoff();

    QFont tickFont(Theme::Font::monospace, Theme::Font::small);
    tickFont.setWeight(QFont::Medium);
    QFont titleFont(Theme::Font::family, Theme::Font::small);
    titleFont.setWeight(QFont::DemiBold);

    QFontMetrics tickFm(tickFont);
    p.setPen(Theme::Colors::textTertiary);

    // Y-axis voltage ticks (left)
    p.setFont(tickFont);
    for (int i = 0; i < GRID_ROW_COUNT; ++i) {
        qreal val = -maxV + i * (2.0 * maxV / (GRID_ROW_COUNT - 1));
        qreal y = dataToScreenY(val, true);
        QString text = QString::number(val, 'f', 1);
        QRect br = tickFm.boundingRect(text);
        p.drawText(QPointF(cr.left() - Theme::Spacing::small - br.width() - Theme::Spacing::small,
                           y + br.height() / 4.0), text);
    }

    // Y-axis current ticks (right)
    for (int i = 0; i < GRID_ROW_COUNT; ++i) {
        qreal val = -maxI + i * (2.0 * maxI / (GRID_ROW_COUNT - 1));
        qreal y = dataToScreenY(val, false);
        QString text = QString::number(val, 'f', 2);
        p.drawText(QPointF(cr.right() + Theme::Spacing::small + Theme::Spacing::small,
                           y + tickFm.height() / 4.0), text);
    }

    // X-axis ticks (bottom)
    int vLines = CYCLE_COUNT * GRID_DIVS_PER_CYCLE + 1;
    for (int i = 0; i < vLines; ++i) {
        qreal t = qreal(i) / GRID_DIVS_PER_CYCLE;
        qreal x = dataToScreenX(t);
        QString text = QString::number(t, 'f', 2);
        QRect br = tickFm.boundingRect(text);
        p.drawText(QPointF(x - br.width() / 2.0,
                           cr.bottom() + Theme::Spacing::small + tickFm.height()), text);
    }

    // Axis titles
    p.setFont(titleFont);
    p.setPen(Theme::Colors::textSecondary);

    // "Voltage (V)" rotated left
    p.save();
    p.translate(cr.left() - Theme::Spacing::medium - Theme::Font::small - tickFm.horizontalAdvance("-000.0"),
                cr.center().y());
    p.rotate(-90);
    p.drawText(QRectF(-50, -10, 100, 20), Qt::AlignCenter, QStringLiteral("Voltage (V)"));
    p.restore();

    // "Current (A)" rotated right
    p.save();
    p.translate(cr.right() + Theme::Spacing::medium + Theme::Font::small + tickFm.horizontalAdvance("00.00") + Theme::Spacing::small,
                cr.center().y());
    p.rotate(90);
    p.drawText(QRectF(-50, -10, 100, 20), Qt::AlignCenter, QStringLiteral("Current (A)"));
    p.restore();

    // "Cycles" bottom center
    p.drawText(QRectF(cr.left(), cr.bottom() + Theme::Spacing::medium + tickFm.height(),
                      cr.width(), 20),
               Qt::AlignHCenter, QStringLiteral("Cycles"));
}

void WaveformPlot::drawTooltipLabels(QPainter &p)
{
    auto *sel = m_model->selectionModel();
    int count = m_model->rowCount();

    QFont font(Theme::Font::monospace, Theme::Font::small);
    font.setWeight(QFont::Medium);
    p.setFont(font);

    for (int i = 0; i < count; ++i) {
        if (!sel->isRowSelected(i))
            continue;

        QString tip = m_model->data(m_model->index(i, 0), Qt::ToolTipRole).toString();
        if (tip.isEmpty())
            continue;

        QColor color = m_model->data(m_model->index(i, 0), Qt::DecorationRole).value<QColor>();
        qreal mag = m_model->data(m_model->index(i, 0), SignalDataModel::MagnitudeRole).toReal();
        qreal phase = m_model->data(m_model->index(i, 0), SignalDataModel::PhaseAngleRole).toReal();
        QString type = m_model->data(m_model->index(i, 0), SignalDataModel::TypeSymbolRole).toString();
        bool isV = (type == "V");
        qreal phaseRad = qDegreesToRadians(phase);

        qreal y0 = mag * qSin(phaseRad);
        qreal sx = dataToScreenX(0);
        qreal sy = dataToScreenY(y0, isV);

        QRectF br = p.fontMetrics().boundingRect(tip);
        QRectF box(sx + Theme::Spacing::medium,
                   sy - br.height() / 2 - Theme::Spacing::small,
                   br.width() + Theme::Spacing::small * 2,
                   br.height() + Theme::Spacing::small * 2);

        p.setPen(QPen(color, Theme::Border::thin));
        p.setBrush(Qt::transparent);
        p.drawRoundedRect(box, Theme::Radius::small, Theme::Radius::small);

        p.setPen(color);
        p.drawText(box, Qt::AlignCenter, tip);
    }
}

// ── Hit testing ─────────────────────────────────────────────────────────────

int WaveformPlot::hitTestWaveform(const QPointF &pos) const
{
    qreal best = HIT_DISTANCE;
    int bestIdx = -1;
    int count = m_model->rowCount();

    for (int si = 0; si < count; ++si) {
        qreal mag = m_model->data(m_model->index(si, 0), SignalDataModel::MagnitudeRole).toReal();
        qreal phase = m_model->data(m_model->index(si, 0), SignalDataModel::PhaseAngleRole).toReal();
        QString type = m_model->data(m_model->index(si, 0), SignalDataModel::TypeSymbolRole).toString();
        bool isV = (type == "V");
        qreal phaseRad = qDegreesToRadians(phase);

        for (int i = 0; i <= 20; ++i) {
            qreal t = qreal(i) * CYCLE_COUNT / 20.0;
            qreal y = mag * qSin(2.0 * M_PI * t + phaseRad);
            qreal sx = dataToScreenX(t);
            qreal sy = dataToScreenY(y, isV);
            qreal d = QLineF(pos, QPointF(sx, sy)).length();
            if (d < best) {
                best = d;
                bestIdx = si;
            }
        }
    }
    return bestIdx;
}

void WaveformPlot::mousePressEvent(QMouseEvent *event)
{
    int idx = hitTestWaveform(event->position());
    auto *sel = m_model->selectionModel();

    if (idx >= 0) {
        sel->select(m_model->index(idx, 0),
                    QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    } else {
        sel->clearSelection();
    }
}
