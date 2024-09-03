#include "util.h"

#include <QDebug>

QPixmap circlePixmap(const QColor &color, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent); // Fill the pixmap with transparent color

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Set the color and draw a solid circle
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(0, 0, size, size);
    painter.end();

    return pixmap;
}

QPixmap twoColorCirclePixmap(const QColor &color1, const QColor &color2, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent); // Fill the pixmap with transparent color

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Set the color and draw a solid circle
    painter.setPen(Qt::NoPen);
    painter.setBrush(color1);
    painter.drawPie(0, 0, size, size, 90 * 16, 270 * 16);
    painter.setBrush(color2);
    painter.drawPie(0, 0, size, size, 0 * 16, -90 * 16);
    painter.drawPie(0, 0, size, size, 0 * 16, +90 * 16);
    painter.end();

    return pixmap;
}

QPixmap rectPixmap(const QColor &color, int width, int height)
{
    QPixmap pixmap(width, height);
    pixmap.fill(Qt::transparent); // Fill the pixmap with transparent color

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Set the color and draw a solid circle
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawRect(0, 0, width, height);
    painter.end();

    return pixmap;
}

QPointF unitvector(qreal angle)
{
    return QPointF(std::cos(angle), std::sin(angle));
}

QPair<qpmu::Float, qpmu::Float> linearRegression(const QVector<double> &x, const QVector<double> &y)
{
    using namespace qpmu;
    Q_ASSERT(x.size() == y.size());
    Q_ASSERT(x.size() > 0);

    Float x_mean = std::accumulate(x.begin(), x.end(), (Float)(0.0)) / x.size();
    Float y_mean = std::accumulate(y.begin(), y.end(), (Float)(0.0)) / y.size();

    Float numerator = 0.0;
    Float denominator = 0.0;

    for (int i = 0; i < x.size(); ++i) {
        numerator += (x[i] - x_mean) * (y[i] - y_mean);
        denominator += (x[i] - x_mean) * (x[i] - x_mean);
    }

    qDebug() << "numerator: " << numerator;
    qDebug() << "denominator: " << denominator;

    Float m = denominator ? numerator / denominator : 0.0;
    Float b = y_mean - (m * x_mean);

    return { m, b }; // Return slope (m) and intercept (b)
}
