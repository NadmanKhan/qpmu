#include "phasorplot.h"
#include "signaldatamodel.h"
#include "theme.h"

#include <QPainter>
#include <QMouseEvent>
#include <QtMath>

PhasorPlot::PhasorPlot(SignalDataModel *model, QWidget *parent) : QWidget(parent), m_model(model)
{
    setMinimumSize(200, 200);
}

QPointF PhasorPlot::center() const
{
    return QPointF(width() / 2.0, height() / 2.0);
}

qreal PhasorPlot::plotRadius() const
{
    return qMin(width(), height()) * RADIUS_RATIO;
}

QPointF PhasorPlot::phasorTip(int i) const
{
    qreal mag = m_model->data(m_model->index(i, 0), SignalDataModel::MagnitudeRole).toReal();
    qreal phase = m_model->data(m_model->index(i, 0), SignalDataModel::PhaseAngleRole).toReal();
    QString type = m_model->data(m_model->index(i, 0), SignalDataModel::TypeSymbolRole).toString();
    qreal cutoff = (type == "V") ? m_model->voltageCutoff() : m_model->currentCutoff();
    qreal norm = mag / cutoff;
    qreal rad = qDegreesToRadians(phase);

    QPointF c = center();
    qreal r = plotRadius();
    return QPointF(c.x() + norm * r * qCos(rad), c.y() - norm * r * qSin(rad));
}

// ---------- painting ----------

void PhasorPlot::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Theme::Colors::surface);

    drawGrid(p);
    drawPhasors(p);
    drawAngleLabels(p);
    drawTooltipLabels(p);
}

void PhasorPlot::drawGrid(QPainter &p)
{
    QPointF c = center();
    qreal r = plotRadius();

    // Concentric circles
    static constexpr qreal radii[] = { 1.0, 0.75, 0.5, 0.25 };
    for (qreal ratio : radii) {
        bool outer = (ratio == 1.0);
        p.setPen(QPen(outer ? Theme::Colors::borderEmphasized : Theme::Colors::borderSubtle,
                      outer ? GRID_LINE_THICK : GRID_LINE_THIN));
        p.drawEllipse(c, ratio * r, ratio * r);
    }

    // Radial lines every 30 degrees
    for (int angle = 0; angle < 360; angle += 30) {
        bool major = (angle % 90 == 0);
        p.setPen(QPen(major ? Theme::Colors::border : Theme::Colors::borderSubtle,
                      major ? GRID_LINE_THIN * 1.25 : GRID_LINE_THIN * 0.66));
        qreal rad = qDegreesToRadians(qreal(angle));
        p.drawLine(c, QPointF(c.x() + r * qCos(rad), c.y() - r * qSin(rad)));
    }

    // Center dot
    p.setPen(QPen(Theme::Colors::textTertiary, GRID_LINE_THIN));
    p.setBrush(Theme::Colors::borderEmphasized);
    p.drawEllipse(c, CENTER_DOT_RADIUS, CENTER_DOT_RADIUS);
}

void PhasorPlot::drawPhasors(QPainter &p)
{
    QPointF c = center();
    int count = m_model->rowCount();

    for (int i = 0; i < count; ++i) {
        QColor color = m_model->data(m_model->index(i, 0), Qt::DecorationRole).value<QColor>();
        QPointF tip = phasorTip(i);

        // Phasor line
        p.setPen(QPen(color, PHASOR_LINE_WIDTH, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(c, tip);

        // Arrowhead
        qreal dx = tip.x() - c.x();
        qreal dy = tip.y() - c.y();
        qreal angle = qAtan2(-dy, dx); // screen Y is flipped
        qreal arrowRad = qDegreesToRadians(ARROW_ANGLE_DEG);
        qreal back = angle + M_PI;

        QPointF a1(tip.x() + ARROW_SIZE * qCos(back + arrowRad),
                   tip.y() - ARROW_SIZE * qSin(back + arrowRad));
        QPointF a2(tip.x() + ARROW_SIZE * qCos(back - arrowRad),
                   tip.y() - ARROW_SIZE * qSin(back - arrowRad));

        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawPolygon(QPolygonF({ tip, a1, a2 }));
    }
}

void PhasorPlot::drawAngleLabels(QPainter &p)
{
    QPointF c = center();
    qreal dist = plotRadius() * LABEL_DISTANCE_RATIO;
    QFont font(Theme::Font::family, Theme::Font::medium);
    font.setWeight(QFont::Medium);
    p.setFont(font);
    p.setPen(Theme::Colors::textTertiary);

    static constexpr int angles[] = { 0, 90, 180, 270 };
    for (int angle : angles) {
        qreal rad = qDegreesToRadians(qreal(angle));
        QPointF pos(c.x() + dist * qCos(rad), c.y() - dist * qSin(rad));
        QString text = QString::number(angle) + QStringLiteral("\u00B0");
        QRectF br = p.fontMetrics().boundingRect(text);
        p.drawText(QPointF(pos.x() - br.width() / 2, pos.y() + br.height() / 4), text);
    }
}

void PhasorPlot::drawTooltipLabels(QPainter &p)
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
        QPointF pos = phasorTip(i);
        QRectF br = p.fontMetrics().boundingRect(tip);
        QRectF box(pos.x() + Theme::Font::large, pos.y() - br.height() / 2 - Theme::Spacing::small,
                   br.width() + Theme::Spacing::small * 2, br.height() + Theme::Spacing::small * 2);

        p.setPen(QPen(color, Theme::Border::thin));
        p.setBrush(Qt::transparent);
        p.drawRoundedRect(box, Theme::Radius::small, Theme::Radius::small);

        p.setPen(color);
        p.drawText(box, Qt::AlignCenter, tip);
    }
}

// ---------- hit testing ----------

qreal PhasorPlot::pointToSegmentDist(const QPointF &pt, const QPointF &a, const QPointF &b)
{
    qreal dx = b.x() - a.x();
    qreal dy = b.y() - a.y();
    qreal lenSq = dx * dx + dy * dy;
    if (lenSq < 1.0)
        return QLineF(pt, a).length();

    qreal t = qBound(0.0, ((pt.x() - a.x()) * dx + (pt.y() - a.y()) * dy) / lenSq, 1.0);
    QPointF closest(a.x() + t * dx, a.y() + t * dy);
    return QLineF(pt, closest).length();
}

int PhasorPlot::hitTestPhasor(const QPointF &pos) const
{
    QPointF c = center();
    qreal best = HIT_RADIUS;
    int bestIdx = -1;

    for (int i = 0; i < m_model->rowCount(); ++i) {
        qreal d = pointToSegmentDist(pos, c, phasorTip(i));
        if (d < best) {
            best = d;
            bestIdx = i;
        }
    }
    return bestIdx;
}

void PhasorPlot::mousePressEvent(QMouseEvent *event)
{
    int idx = hitTestPhasor(event->position());
    auto *sel = m_model->selectionModel();

    if (idx >= 0) {
        sel->select(m_model->index(idx, 0),
                    QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    } else {
        sel->clearSelection();
    }
}
