#include "main_window.h"
#include "main_page_interface.h"
#include "phasor_monitor.h"
#include "settings_widget.h"
#include "data_processor.h"
#include "util.h"

#include <QIcon>
#include <QPushButton>
#include <QToolButton>
#include <QLayout>
#include <QGridLayout>
#include <QStackedWidget>
#include <QDockWidget>
#include <QLabel>
#include <QSettings>
#include <QToolBox>
#include <QStackedWidget>
#include <QToolBar>
#include <QString>
#include <QIcon>
#include <qboxlayout.h>
#include <qnamespace.h>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{

    auto toolBar = new QToolBar();
    auto mainPageStack = new QStackedWidget();
    auto sidePanel = new QToolBox();
    auto toggleSidePanelAction = new QAction();
    auto goBackAction = new QAction();

    { /// Central widget
        auto cwidget = new QWidget(this);
        setCentralWidget(cwidget);
        auto layout = new QHBoxLayout(cwidget);
        layout->setContentsMargins(QMargins(0, 0, 0, 0));
    }

    { /// Status bar
        statusBar(); /// first call to `statusBar()` creates and sets it
        statusBar()->setSizeGripEnabled(true);
        statusBar()->setContentsMargins(QMargins(5, 0, 0, 0));

        /// 1. Label for sampling indicattor=
        statusBar()->addPermanentWidget(new QLabel("Sampling", statusBar()));

        /// 2. Sampling indicator
        auto samplingIndicator = new QLabel(statusBar());
        statusBar()->addPermanentWidget(samplingIndicator);

        auto updateSamplingIndicator = [=](bool isConnected) {
            static const auto redCircle = circlePixmap(QColor("#dc143c"), 12);
            static const auto greenCircle = circlePixmap(QColor("#22bb45"), 12);
            if (isConnected) {
                samplingIndicator->setPixmap(greenCircle);
            } else {
                samplingIndicator->setPixmap(redCircle);
            }
        };

        connect(APP->dataProcessor(), &DataProcessor::sampleSourceConnectionChanged,
                updateSamplingIndicator);
        updateSamplingIndicator(false);

        /// 3. Spacer
        auto spacer = new QWidget(statusBar());
        // spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
        statusBar()->addPermanentWidget(spacer, 1);

        /// 4. Time and date
        auto timeLabel = new QLabel(statusBar());
        statusBar()->addPermanentWidget(timeLabel);

        /// 5. Separator
        auto separator3 = new QFrame(statusBar());
        separator3->setFrameStyle(QFrame::VLine | QFrame::Raised);
        statusBar()->addPermanentWidget(separator3);

        /// 6. Date
        auto dateLabel = new QLabel(statusBar());
        statusBar()->addPermanentWidget(dateLabel);

        { /// Set up date and time labels

            /// Make font monospace
            auto font = dateLabel->font();
            font.setFamily("'Source Sans Pro', 'Roboto Mono', 'Fira Code', Consolas, Monospace");
            dateLabel->setFont(font);
            timeLabel->setFont(font);

            /// Update date and time at every timer tick (check `APP->timer()` for the interval)
            connect(APP->timer(), &QTimer::timeout, [=] {
                auto now = QDateTime::currentDateTime();
                dateLabel->setText(now.date().toString());
                timeLabel->setText(now.time().toString());
            });
        }

        /// Must call show to make the status bar visible
        statusBar()->show();
    }

    { /// Side panel (tool box)
        static_cast<QHBoxLayout *>(centralWidget()->layout())->addWidget(sidePanel);
        sidePanel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
        sidePanel->setFrameStyle(QFrame::Panel | QFrame::Raised);
        sidePanel->hide();
    }

    { /// Tool bar
        addToolBar(Qt::LeftToolBarArea, toolBar);
        toolBar->setMovable(false);

        { /// Back button
            toolBar->addAction(goBackAction);
            goBackAction->setIcon(QIcon(":/back.png"));
            goBackAction->setText("Back");
            goBackAction->setEnabled(false);
            connect(mainPageStack, &QStackedWidget::currentChanged,
                    [=](int index) { goBackAction->setEnabled(index >= 1); });
            connect(goBackAction, &QAction::triggered, [=] {
                auto i = mainPageStack->currentIndex();
                if (i <= 0) {
                    return;
                }
                mainPageStack->setCurrentIndex(i - 1);
                mainPageStack->removeWidget(mainPageStack->widget(i));
            });
        }

        { /// Toggle side panel button
            toolBar->addAction(toggleSidePanelAction);
            toggleSidePanelAction->setIcon(QIcon(":/more.png"));
            toggleSidePanelAction->setText("Menu");
            toggleSidePanelAction->setCheckable(true);
            connect(toggleSidePanelAction, &QAction::toggled, sidePanel, &QWidget::setVisible);
            toggleSidePanelAction->setVisible(false);
        }
    }

    { /// Main page stack
        static_cast<QHBoxLayout *>(centralWidget()->layout())->addWidget(mainPageStack, 1);
        mainPageStack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        mainPageStack->setContentsMargins(QMargins(20, 10, 20, 10));

        connect(mainPageStack, &QStackedWidget::currentChanged, [=](int index) {
            /// Clear side-panel items, and hide it
            for (int i = 0; i < sidePanel->count(); ++i) {
                sidePanel->removeItem(i);
            }
            toggleSidePanelAction->setChecked(false);
            toggleSidePanelAction->setVisible(false);

            /// If the current page has side-panel items, add them
            if (auto page = qobject_cast<MainPageInterface *>(mainPageStack->widget(index))) {
                const auto &items = page->sidePanelItems();
                for (const auto &[widget, title, icon] : items) {
                    if (icon.isNull()) {
                        sidePanel->addItem(widget, title);
                    } else {
                        sidePanel->addItem(widget, icon, title);
                    }
                }
                toggleSidePanelAction->setVisible(items.count() > 0);
            }
        });
    }

    { /// Home page widget (first view)
        auto homePage = new QWidget(mainPageStack);
        mainPageStack->setCurrentIndex(mainPageStack->addWidget(homePage));
        homePage->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        auto homeGrid = new QGridLayout(homePage);
        homeGrid->setSpacing(10);

        using HomeOption = std::tuple<QString, QString, QWidget *>;
        QList<HomeOption> homeOptions;
        homeOptions.append(HomeOption{ "Monitor", ":/monitor.png", new PhasorMonitor() });
        homeOptions.append(HomeOption{ "Settings", ":/control-panel.png", new SettingsWidget() });

        const int rowCount = 2;
        const int colCount = 2;

        /// Add option buttons to home widget
        for (int i = 0; i < (int)homeOptions.size(); ++i) {
            const auto [title, iconUrl, page] = homeOptions[i];
            page->setParent(this);
            auto button = new QPushButton(homePage);
            auto label = new QLabel(homePage);

            { /// Button with icon
                button->setIcon(QIcon(iconUrl));
                auto size = QSize(80, 80);
                button->setMinimumSize(size);
                button->setMaximumSize(size);
                button->setIconSize(size * 0.9);
                connect(button, &QPushButton::clicked, [=] {
                    const auto [title, iconUrl, page] = homeOptions[i];
                    auto index = mainPageStack->addWidget(page);
                    mainPageStack->setCurrentIndex(index);
                    page->show();
                });
            }

            { /// Label
                label->setText(title);
                label->setAlignment(Qt::AlignCenter);
            }

            { /// Layout to hold button and label
                auto vbox = new QVBoxLayout();
                vbox->addWidget(button);
                vbox->setAlignment(button, Qt::AlignCenter);
                vbox->addWidget(label);
                vbox->setAlignment(label, Qt::AlignTop | Qt::AlignHCenter);
                homeGrid->addLayout(vbox, (i / rowCount), (i % colCount));
                homeGrid->setAlignment(vbox, Qt::AlignCenter);
            }
        }
    }

    resize(1000, 500);
}
