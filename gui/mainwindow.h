#pragma once

#include <QMainWindow>

class QLabel;
class QStackedWidget;
class QPushButton;

class SignalDataModel;
class Screen;
class ContextMenuPanel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(SignalDataModel *model, QWidget *parent = nullptr);

    void setLiveMode(bool live);
    void setLastSampleTime(const QString &time);
    void setSystemFrequency(qreal freq);

    void pushScreen(Screen *screen);
    void popScreen();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateToolbar();
    void updateConnectionIndicator();

    SignalDataModel *m_model;

    // Navigation
    QStackedWidget *m_screenStack;

    // Toolbar
    QLabel *m_titleLabel;
    QPushButton *m_backButton;
    QPushButton *m_menuButton;

    // Context menu panel
    ContextMenuPanel *m_contextPanel;

    // Status bar
    QWidget *m_statusBar;
    QLabel *m_connectionDot;
    QLabel *m_connectionLabel;
    QWidget *m_liveMetrics;
    QLabel *m_sampleTimeValue;
    QLabel *m_freqValue;
    QLabel *m_rateValue;
    QLabel *m_utcLabel;

    bool m_liveMode = false;
};
