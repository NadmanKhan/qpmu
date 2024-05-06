#ifndef QPMU_GUI_UTIL_H
#define QPMU_GUI_UTIL_H

#include <QPainter>
#include <QPixmap>
#include <QIcon>

#include <cmath>

QPixmap circlePixmap(const QColor &color, int size);

QPixmap twoColorCirclePixmap(const QColor &color1, const QColor &color2, int size);


QPointF unitvector(qreal angle);

#endif // QPMU_GUI_UTIL_H