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
    void updateStatusIndicator();

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
    QLabel *m_statusDot;
    QLabel *m_statusText;
    QWidget *m_statusPill;
    QLabel *m_timeValue;
    QLabel *m_rateValue;
    QLabel *m_freqValue;

    bool m_liveMode = false;
};
