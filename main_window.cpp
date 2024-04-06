#include "main_window.h"

MainWindow::MainWindow(Worker *worker, QWidget *parent) : QMainWindow{ parent }
{
    // init members
    m_stack = new QStackedWidget(this);
    m_timer = new QTimer(this);
    m_timer->setInterval(20);

    { // modify `this` (MainWindow)
        setCentralWidget(m_stack);
        // first call to `statusBar()` automatically sets it
        statusBar()->setSizeGripEnabled(true);
    }

    { // statusbar

        auto timeLabel = new QLabel(statusBar());
        statusBar()->addPermanentWidget(timeLabel);

        auto vline = new QFrame(statusBar());
        vline->setFrameStyle(QFrame::VLine | QFrame::Raised);
        statusBar()->addPermanentWidget(vline);

        auto dateLabel = new QLabel(statusBar());
        statusBar()->addPermanentWidget(dateLabel);

        auto font = dateLabel->font();
        font.setFamily(QStringLiteral(
                "'Source Sans Pro', 'Roboto Mono', 'Fira Code', Consolas, Monospace"));

        dateLabel->setFont(font);
        timeLabel->setFont(font);
        connect(m_timer, &QTimer::timeout, [=] {
            auto now = QDateTime::currentDateTime();
            dateLabel->setText(now.date().toString());
            timeLabel->setText(now.time().toString());
        });
        m_timer->start();
        statusBar()->show();
    }

    auto grid = new QGridLayout();
    grid->setSpacing(10);

    auto firstView = new QWidget(this);
    firstView->setLayout(grid);

    m_stack->setCurrentIndex(m_stack->addWidget(firstView));

    using OptionModel = std::tuple<QString, QString, QWidget *>;
    QList<OptionModel> optionsModel;
    optionsModel.append(OptionModel{ QStringLiteral("Monitor Phasors"),
                                     QStringLiteral(":/images/polar-chart.png"),
                                     new PhasorView(m_timer, worker, this) });
    optionsModel.append(OptionModel{ QStringLiteral("Monitor Waveforms"),
                                     QStringLiteral(":/images/wave-graph.png"),
                                     new WaveformView(m_timer, worker, this) });

    const int rowCount = 2;
    const int colCount = 2;
    for (int i = 0; i < (int)optionsModel.size(); ++i) {
        const auto [title, iconUrl, targetWidget] = optionsModel[i];

        auto button = new QPushButton(this);
        button->setIcon(QIcon(iconUrl));
        auto size = QSize(120, 120);
        button->setMinimumSize(size);
        button->setMaximumSize(size);
        button->setIconSize(size * 0.9);
        connect(button, &QPushButton::clicked, [=] {
            const auto [title, iconUrl, targetWidget] = optionsModel[i];
            auto index = m_stack->addWidget(targetWidget);
            m_stack->setCurrentIndex(index);
            targetWidget->show();
        });

        auto label = new QLabel(this);
        label->setText(title);
        label->setAlignment(Qt::AlignCenter);

        auto vbox = new QVBoxLayout();
        vbox->addWidget(button);
        vbox->setAlignment(button, Qt::AlignCenter);
        vbox->addWidget(label);
        vbox->setAlignment(label, Qt::AlignTop | Qt::AlignHCenter);

        grid->addLayout(vbox, (i / rowCount), (i % colCount));
        grid->setAlignment(vbox, Qt::AlignCenter);
    }
}
