#ifndef APP_H
#define APP_H

#include <QApplication>
#include <QMetaType>
#include <QThread>

#include "AdcSampleModel.h"

class App: public QApplication
{
Q_OBJECT

public:
    explicit App(int &argc, char **argv);


private:
    QThread *m_modelThread = nullptr;
    AdcSampleModel *m_adcSampleModel = nullptr;

public:
    [[nodiscard]] AdcSampleModel *adcSampleModel() const;
};


#endif //APP_H
