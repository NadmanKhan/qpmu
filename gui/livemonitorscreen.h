#pragma once

#include "screen.h"

class QSplitter;
class QStackedWidget;
class QTabBar;
class QTableView;

class SignalDataModel;
class PhasorPlot;
class WaveformPlot;
class ContextItemModel;

class LiveMonitorScreen : public Screen
{
    Q_OBJECT

public:
    explicit LiveMonitorScreen(SignalDataModel *model, QWidget *parent = nullptr);

    QString title() const override;
    ContextItemModel *contextModel() const override;

private:
    void buildContextModel();

    SignalDataModel *m_model;
    ContextItemModel *m_contextModel;

    PhasorPlot *m_phasorPlot;
    WaveformPlot *m_waveformPlot;
    QStackedWidget *m_plotStack;
    QTabBar *m_tabBar;
    QSplitter *m_splitter;
    QTableView *m_tableView;
};
