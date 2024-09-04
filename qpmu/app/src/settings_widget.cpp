#include "settings_widget.h"
#include "app.h"
#include "data_processor.h"
#include "util.h"
#include "qpmu/util.h"
#include "qpmu/defs.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QRadioButton>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QLabel>
#include <QLineEdit>
#include <QMargins>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QWidget>
#include <QHeaderView>
#include <qabstractitemmodel.h>

SettingsWidget::SettingsWidget(QWidget *parent) : QTabWidget(parent)
{
    hide();
    setTabPosition(QTabWidget::West);

    addTab(createSampleSourcePage(), "Sample Source");
    addTab(createCalibrationPage(), "Calibrate");
}

QWidget *SettingsWidget::createSampleSourcePage()
{

    auto settings = APP->settings();

    auto dialog = new QWidget();
    auto layout = new QVBoxLayout(dialog);

    {
        auto hostEdit = new QLineEdit();
        auto portEdit = new QLineEdit();
        auto buttonBox = new QDialogButtonBox();

        { /// Form Layout
            auto formLayout = new QFormLayout();
            layout->addLayout(formLayout);
            formLayout->setContentsMargins(QMargins(10, 10, 10, 10));

            { /// Host IP
                formLayout->addRow("Host IP", hostEdit);
                auto ipPattern = QString("^%1\\.%1\\.%1\\.%1$")
                                         .arg("(25[0-5]|2[0-4][0-9]|[0-1]?[0-9]{1,2})");
                auto ipValidator = new QRegularExpressionValidator(QRegularExpression(ipPattern));
                hostEdit->setValidator(ipValidator);
                hostEdit->setInputMask(QStringLiteral("000.000.000.000;_"));
                hostEdit->setPlaceholderText("xxx.xxx.xxx.xxx");
                hostEdit->setText(
                        settings->get(Settings::list.sampling.inputSource.host).toString());
                hostEdit->setFixedWidth(150);
            }

            { /// Port
                auto portValidator = new QIntValidator(0, 65535);
                formLayout->addRow("Port", portEdit);
                portEdit->setValidator(portValidator);
                portEdit->setPlaceholderText("0-65535");
                portEdit->setText(
                        settings->get(Settings::list.sampling.inputSource.port).toString());
                portEdit->setFixedWidth(150);
            }
        }

        { /// Button box
            layout->addWidget(buttonBox);
            auto saveButton = buttonBox->addButton(QDialogButtonBox::Save);
            auto resetButton = buttonBox->addButton(QDialogButtonBox::Reset);
            auto restoreDefaultsButton = buttonBox->addButton(QDialogButtonBox::RestoreDefaults);

            { /// Save button
                saveButton->setEnabled(false);
                connect(hostEdit, &QLineEdit::textChanged, [=] {
                    saveButton->setEnabled(portEdit->hasAcceptableInput()
                                           && hostEdit->hasAcceptableInput());
                });
                connect(portEdit, &QLineEdit::textChanged, [=] {
                    saveButton->setEnabled(portEdit->hasAcceptableInput()
                                           && hostEdit->hasAcceptableInput());
                });
                connect(saveButton, &QAbstractButton::clicked, [=] {
                    settings->set(Settings::list.sampling.inputSource.host, hostEdit->text());
                    settings->set(Settings::list.sampling.inputSource.port, portEdit->text());
                });
            }

            { /// Reset button
                connect(resetButton, &QAbstractButton::clicked, [=] {
                    hostEdit->setText(
                            settings->get(Settings::list.sampling.inputSource.host).toString());
                    portEdit->setText(
                            settings->get(Settings::list.sampling.inputSource.port).toString());
                });
            }

            { /// Restore defaults button
                connect(restoreDefaultsButton, &QAbstractButton::clicked, [=] {
                    hostEdit->setText(
                            Settings::list.sampling.inputSource.host.defaultValue.toString());
                    portEdit->setText(
                            Settings::list.sampling.inputSource.port.defaultValue.toString());
                });
            }
        }
    }

    return dialog;
}

QWidget *SettingsWidget::createCalibrationPage()
{
    using namespace qpmu;
    auto page = new QTabWidget();
    auto settings = APP->settings();

    for (USize i = 0; i < SignalCount; ++i) {
        auto widget = createSignalCalibrationWidget(i);
        page->addTab(widget, SignalNames[i]);
        auto color = settings->get(Settings::list.appearance.signalColors[i]).value<QColor>();
        page->setTabIcon(i, QIcon(rectPixmap(color, 4, 10)));
    }

    return page;
}

QWidget *SettingsWidget::createSignalCalibrationWidget(qpmu::USize signalIndex)
{
    using namespace qpmu;
    auto settings = APP->settings();

    auto dialog = new QWidget();
    auto layout = new QVBoxLayout(dialog);

    {
        auto slopeEdit = new QLineEdit();
        auto interceptEdit = new QLineEdit();
        auto table = new QTableWidget();
        auto buttonBox = new QDialogButtonBox();

        { /// Results layout
            auto resultsLayout = new QFormLayout();
            layout->addLayout(resultsLayout);

            { /// Result slope
                resultsLayout->addRow("Slope", slopeEdit);
                slopeEdit->setText(
                        settings->get(Settings::list.sampling.calibration[signalIndex * 2 + 0])
                                .toString());
                slopeEdit->setReadOnly(true);
                slopeEdit->setFixedWidth(100);
            }

            { /// Result intercept
                resultsLayout->addRow("Intercept", interceptEdit);
                interceptEdit->setText(
                        settings->get(Settings::list.sampling.calibration[signalIndex * 2 + 1])
                                .toString());
                interceptEdit->setReadOnly(true);
                interceptEdit->setFixedWidth(100);
            }
        }

        { /// Table
            layout->addWidget(table);
            table->setRowCount(0);
            table->setColumnCount(3); // Sample, Actual, Delete

            { /// header
                auto headerLabels = QStringList();
                headerLabels << QStringLiteral("Sample Value (Press to Update)");
                headerLabels << QStringLiteral("Actual %1 (%2)")
                                        .arg(SignalTypeNames[signalIndex / 3])
                                        .arg(SignalTypeUnitSymbols[signalIndex / 3]);
                headerLabels << QStringLiteral("Delete");
                table->setHorizontalHeaderLabels(headerLabels);
                table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
                table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
                table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
                table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
            }

            connect(table, &QTableWidget::itemPressed, [=](QTableWidgetItem *item) {
                if (item->column() == 0) {
                    /// Update sample magnitude
                    auto sampleMagnitudes = APP->dataProcessor()->channelMagnitudes();
                    item->setText(QString::number(sampleMagnitudes[signalIndex], 'f', 2));
                }
            });

            { /// Add button
                auto addRowButton = new QPushButton(QStringLiteral("+"));
                table->insertRow(0);
                table->setCellWidget(0, 0, addRowButton);
                table->setSpan(0, 0, 1, 3);

                addRowButton->setToolTip(QStringLiteral("Add a new row"));
                addRowButton->setContentsMargins(QMargins(10, 0, 10, 0));

                connect(addRowButton, &QPushButton::clicked, [=] {
                    const auto row = table->rowCount() - 1; // Last row is the add button
                    table->insertRow(row);

                    { /// Sample magnitude cell
                        auto sampleItem = new QTableWidgetItem();
                        table->setItem(row, 0, sampleItem);
                        sampleItem->setTextAlignment(Qt::AlignRight);
                        sampleItem->setFlags(Qt::NoItemFlags | Qt::ItemIsEnabled);
                        sampleItem->setText(QStringLiteral("0.00"));
                        sampleItem->setToolTip(
                                QStringLiteral("Press to update the sample magnitude"));
                    }

                    { /// Actual magnitude cell
                        auto actualItem = new QTableWidgetItem();
                        table->setItem(row, 1, actualItem);
                        actualItem->setTextAlignment(Qt::AlignRight);
                        actualItem->setFlags(Qt::NoItemFlags | Qt::ItemIsEnabled
                                             | Qt::ItemIsSelectable | Qt::ItemIsEditable);
                        actualItem->setText(QStringLiteral("0.00"));
                        actualItem->setToolTip(
                                QStringLiteral("Enter the actual magnitude of the sample"));
                    }

                    { /// Delete button
                        auto deleteButton = new QPushButton();
                        table->setCellWidget(row, 2, deleteButton);

                        deleteButton->setIcon(QIcon(QStringLiteral(":/delete.png")));
                        deleteButton->setToolTip(QStringLiteral("Delete this row"));
                        deleteButton->setContentsMargins(QMargins(5, 0, 5, 0));

                        connect(deleteButton, &QPushButton::clicked,
                                [=] { table->removeRow(row); });
                    }
                });
            }
        }

        { /// Button box
            layout->addWidget(buttonBox);
            auto calibrateButton = buttonBox->addButton(QDialogButtonBox::Apply);
            auto saveButton = buttonBox->addButton(QDialogButtonBox::Save);
            auto resetButton = buttonBox->addButton(QDialogButtonBox::Reset);
            auto clearButton = buttonBox->addButton(QDialogButtonBox::RestoreDefaults);

            { /// Calibrate button
                calibrateButton->setEnabled(false);
                calibrateButton->setText(QStringLiteral("Calibrate"));

                connect(table->model(), &QAbstractItemModel::rowsInserted,
                        [=] { calibrateButton->setEnabled(table->rowCount() > 1); });
                connect(table->model(), &QAbstractItemModel::rowsRemoved,
                        [=] { calibrateButton->setEnabled(table->rowCount() > 1); });

                connect(calibrateButton, &QPushButton::clicked, [=] {
                    auto rows = table->rowCount();
                    auto samples = std::vector<Float>();
                    auto actuals = std::vector<Float>();

                    for (int row = 0; row < rows; ++row) {
                        auto sampleItem = table->item(row, 0);
                        auto actualItem = table->item(row, 1);

                        if (sampleItem && actualItem) {
                            samples.push_back(sampleItem->text().toDouble());
                            actuals.push_back(actualItem->text().toDouble());
                        }
                    }

                    auto [slope, intercept] = util::linearRegression(samples, actuals);
                    slopeEdit->setText(QString::number(slope, 'f', 2));
                    interceptEdit->setText(QString::number(intercept, 'f', 2));
                });
            }

            { /// Save button
                saveButton->setEnabled(false);

                auto readyToSave = [=] {
                    const auto &slopeText = slopeEdit->text();
                    bool ok1 = false;
                    auto slope = slopeText.toDouble(&ok1);

                    const auto &interceptText = interceptEdit->text();
                    auto ok2 = false;
                    interceptText.toDouble(&ok2);

                    return ok1 && ok2 && !slopeText.isEmpty() && !interceptText.isEmpty()
                            && slope != 0.0;
                };
                connect(slopeEdit, &QLineEdit::textChanged,
                        [=] { saveButton->setEnabled(readyToSave()); });
                connect(interceptEdit, &QLineEdit::textChanged,
                        [=] { saveButton->setEnabled(readyToSave()); });

                connect(saveButton, &QPushButton::clicked, [=] {
                    auto slope = slopeEdit->text().toDouble();
                    auto intercept = interceptEdit->text().toDouble();
                    settings->set(Settings::list.sampling.calibration[signalIndex * 2 + 0], slope);
                    settings->set(Settings::list.sampling.calibration[signalIndex * 2 + 1],
                                  intercept);
                });
            }

            { /// Reset button
                connect(resetButton, &QPushButton::clicked, [=] {
                    slopeEdit->setText(
                            settings->get(Settings::list.sampling.calibration[signalIndex * 2 + 0])
                                    .toString());
                    interceptEdit->setText(
                            settings->get(Settings::list.sampling.calibration[signalIndex * 2 + 1])
                                    .toString());
                });
            }

            { /// Clear button
                clearButton->setText(QStringLiteral("Clear Table"));
                connect(clearButton, &QPushButton::clicked, [=] {
                    while (table->rowCount() > 1) {
                        table->removeRow(0);
                    }
                });
            }
        }
    }

    return dialog;
}