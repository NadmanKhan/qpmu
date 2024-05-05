#include "util.h"

QIcon circleIcon(const QColor &color, int size)
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

    return QIcon(pixmap);
}

QIcon twoColorCircleIcon(const QColor &color1, const QColor &color2, int size)
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

    return QIcon(pixmap);
}
