#ifndef APPMAINWINDOW_H
#define APPMAINWINDOW_H

#include "appcentralwidget.h"
#include "appmainmenu.h"
#include "appdockwidget.h"
#include "appstatusbar.h"
#include "apptoolbar.h"
#include "phasorwaveview.h"
#include "phasorpolarview.h"
#include "pmu.h"

#include <QMainWindow>

class AppMainWindow : public QMainWindow {
    Q_OBJECT

public:
    static AppMainWindow* ptr();

private:
    AppMainWindow();

    static AppMainWindow* m_ptr;
    PhasorWaveView* m_waveView;
    PhasorPolarView* m_polarView;
};
#endif // APPMAINWINDOW_H
