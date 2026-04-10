#pragma once

#include <QColor>
#include <QFont>
#include <QString>

namespace Theme {

namespace Colors {
// Backgrounds
inline const QColor background(0x05, 0x08, 0x10);
inline const QColor surface(0x0a, 0x0e, 0x14);
inline const QColor surfaceElevated(0x1b, 0x28, 0x38);
inline const QColor surfaceHover(0x1a, 0x26, 0x34);
inline const QColor surfacePressed(0x0f, 0x18, 0x23);

// Primary brand
inline const QColor primary(0x06, 0xd6, 0xa0);
inline const QColor error(0xe6, 0x39, 0x46);
inline const QColor info(0x45, 0xb7, 0xd1);

// Text hierarchy
inline const QColor textPrimary(0xd0, 0xda, 0xe8);
inline const QColor textSecondary(0x8b, 0xa3, 0xbe);
inline const QColor textTertiary(0x6b, 0x8c, 0xae);

// Borders
inline const QColor border(0x2a, 0x3f, 0x5f);
inline const QColor borderEmphasized(0x3d, 0x5a, 0x80);
inline const QColor borderSubtle(0x1b, 0x28, 0x38);

// Signal colors (VA, VB, VC, IA, IB, IC)
inline const QColor signal[] = {
    QColor(0xff, 0x6b, 0x6b), // VA - Coral red
    QColor(0x4e, 0xcd, 0xc4), // VB - Teal
    QColor(0x45, 0xb7, 0xd1), // VC - Sky blue
    QColor(0xff, 0xd9, 0x3d), // IA - Golden yellow
    QColor(0xc5, 0x6c, 0xf0), // IB - Purple
    QColor(0x95, 0xe1, 0xd3), // IC - Mint
};
} // namespace Colors

namespace Sizing {
inline constexpr int toolbarHeight = 55;
inline constexpr int statusBarHeight = 72;
inline constexpr int small = 30;
inline constexpr int medium = 44;
inline constexpr int large = 54;
} // namespace Sizing

namespace Spacing {
inline constexpr int tiny = 3;
inline constexpr int small = 8;
inline constexpr int medium = 14;
inline constexpr int large = 24;
inline constexpr int huge = 30;
} // namespace Spacing

namespace Font {
inline constexpr int tiny = 10;
inline constexpr int small = 11;
inline constexpr int medium = 13;
inline constexpr int normal = 15;
inline constexpr int large = 18;
inline constexpr int huge = 22;

inline const QString family = QStringLiteral("Segoe UI");
inline const QString monospace = QStringLiteral("Consolas");
} // namespace Font

namespace Radius {
inline constexpr int small = 6;
inline constexpr int medium = 8;
inline constexpr int large = 10;
} // namespace Radius

namespace Border {
inline constexpr int thin = 1;
inline constexpr int medium = 2;
inline constexpr int thick = 2;
} // namespace Border

inline QColor withAlpha(QColor c, int alpha)
{
    c.setAlpha(alpha);
    return c;
}

} // namespace Theme
