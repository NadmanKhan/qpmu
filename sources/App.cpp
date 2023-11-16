#include "App.h"

App::App(int &argc, char **argv)
    : QApplication(argc, argv)
{
    (void)qRegisterMetaType<AdcSampleVector>("SampleVector");

    m_modelThread = new QThread();

    m_adcSampleModel = new AdcSampleModel();

    m_adcSampleModel->moveToThread(m_modelThread);
    Q_ASSERT(connect(m_modelThread, &QThread::finished,
                     m_adcSampleModel, &AdcSampleModel::deleteLater));
    m_modelThread->start();
}

AdcSampleModel *App::adcSampleModel() const
{
    return m_adcSampleModel;
}
