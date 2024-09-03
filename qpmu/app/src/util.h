#ifndef QPMU_GUI_UTIL_H
#define QPMU_GUI_UTIL_H

#include <QPainter>
#include <QPixmap>
#include <QIcon>
#include <QPair>
#include <QVector>

#include <cmath>

#include "qpmu/common.h"

QPixmap circlePixmap(const QColor &color, int size);

QPixmap twoColorCirclePixmap(const QColor &color1, const QColor &color2, int size);

QPixmap rectPixmap(const QColor &color, int width, int height);

QPointF unitvector(qreal angle);

QPair<qpmu::Float, qpmu::Float> linearRegression(const QVector<double> &x, const QVector<double> &y);

#endif // QPMU_GUI_UTIL_H