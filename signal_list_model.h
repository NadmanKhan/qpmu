#ifndef SIGNAL_LIST_MODEL_H
#define SIGNAL_LIST_MODEL_H

#include <QList>
#include <QString>
#include <utility>

constexpr auto signalInfoListModel = std::array<std::tuple<const char *, const char *>, 6>{
    std::make_tuple(("VA"), ("#000000")), std::make_tuple(("VB"), ("#ff0000")),
    std::make_tuple(("VC"), ("#00ffff")), std::make_tuple(("IA"), ("#ffdd00")),
    std::make_tuple(("IB"), ("#0000ff")), std::make_tuple(("IC"), ("#00ff00"))
};
#endif // SIGNAL_LIST_MODEL_H
