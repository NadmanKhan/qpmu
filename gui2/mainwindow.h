#pragma once

#include <QMainWindow>

class QLabel;
class QSplitter;
class QStackedWidget;
class QTabBar;
class QTableView;
class QPushButton;

class SignalDataModel;
class PhasorPlot;
class WaveformPlot;
class ContextMenuPanel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(SignalDataModel *model, QWidget *parent = nullptr);

    void setLiveMode(bool live);
    void setLastSampleTime(const QString &time);
    void setSystemFrequency(qreal freq);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupToolbar();
    void setupCentralArea();
    void setupStatusBar();

    void updateStatusIndicator();

    SignalDataModel *m_model;

    // Plots
    PhasorPlot *m_phasorPlot;
    WaveformPlot *m_waveformPlot;
    QStackedWidget *m_plotStack;
    QTabBar *m_tabBar;
    QSplitter *m_splitter;
    QTableView *m_tableView;

    // Toolbar
    QPushButton *m_menuButton;

    // Context menu panel
    ContextMenuPanel *m_contextPanel;

    // Status bar
    QWidget *m_statusBar;
    QLabel *m_statusDot;
    QLabel *m_statusText;
    QWidget *m_statusPill;
    QLabel *m_timeValue;
    QLabel *m_rateValue;
    QLabel *m_freqValue;

    bool m_liveMode = false;
};
