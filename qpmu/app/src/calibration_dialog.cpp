#include "app.h"
#include "settings.h"
#include "qpmu/common.h"
#include "router.h"
#include "util.h"
#include "timeout_notifier.h"
#include "calibration_dialog.h"

#include <QAction>
#include <QIcon>
#include <QDialog>
#include <QLineEdit>
#include <QDateTime>
#include <QDoubleValidator>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QInputDialog>
#include <QString>
#include <QTableWidgetItem>
#include <QAbstractItemModel>
#include <QHeaderView>
#include <qnamespace.h>
#include <qpushbutton.h>
#include <qtablewidget.h>

CalibrationDialog::CalibrationDialog(QWidget *parent) : QDialog(parent)
{
    using namespace qpmu;

    auto &settings = *APP->settings();

    /// Dialog attributes
    /// ---

    setModal(true);
    setWindowModality(Qt::WindowModal);
    setWindowTitle("Calibration");
    auto outerLayout = new QVBoxLayout(this);

    auto firstLineLayout = new QHBoxLayout();
    auto secondLineLayout = new QHBoxLayout();
    auto thirdLineLayout = new QHBoxLayout();
    outerLayout->addLayout(firstLineLayout);
    outerLayout->addLayout(secondLineLayout);
    outerLayout->addLayout(thirdLineLayout);

    /// Which signal to calibrate
    /// ---

    /// Layout for radio buttons
    auto radioLayout = new QVBoxLayout();
    secondLineLayout->addLayout(radioLayout);

    /// Radio buttons
    m_signalChoiceButtonGroup = new QButtonGroup(this);
    m_signalChoiceButtonGroup->setExclusive(true);
    QRadioButton *signalTypeRadioButtons[CountSignals];
    for (USize i = 0; i < CountSignals; ++i) {
        auto radio = signalTypeRadioButtons[i] = new QRadioButton(Signals[i].name, this);
        radioLayout->addWidget(radio);
        m_signalChoiceButtonGroup->addButton(radio, i);
        auto color =
                QColor(settings.get(Settings::list.appearance.signalColors[i]).value<QColor>());
        auto icon = QIcon(rectPixmap(color, 4, 10));
        radio->setIcon(icon);
    }
    radioLayout->addStretch(1);

    /// Magnitude validators
    for (USize i = 0; i < CountSignalTypes; ++i) {
        Float upper =
                settings.get(Settings::list.fundamentals.powerSource.maxMagnitdue[i]).toFloat();
        m_magnitudeValidators[i] = new QDoubleValidator(0, upper, 2, this);
    }

    /// Table
    /// ---

    /// Layout for table
    auto tableLayout = new QVBoxLayout();
    secondLineLayout->addLayout(tableLayout);

    /// Init table
    m_table = new QTableWidget(0, 3, this);
    tableLayout->addWidget(m_table);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setFocusPolicy(Qt::NoFocus);

    /// Header
    m_table->setHorizontalHeaderLabels(QStringList() << "Sample Magnitude"
                                                     << "Actual Magnitude"
                                                     << "");
    signalTypeRadioButtons[0]->click();

    /// Connections
    connect(m_table, &QTableWidget::cellActivated, this, &CalibrationDialog::handleCellActivation);

    /// Add row button
    /// ---

    m_addRowButton = new QPushButton("+", this);
    tableLayout->addWidget(m_addRowButton);
    m_addRowButton->setToolTip("Add a new row");
    connect(m_addRowButton, &QPushButton::clicked, this, &CalibrationDialog::addRow);

    /// Timeout notifier
    /// ---

    m_recordingEndNotifier =
            new TimeoutNotifier(APP->timer(), SecondsForSampleRecording * 1000, this);
    connect(m_recordingEndNotifier, &TimeoutNotifier::timeout, this,
            &CalibrationDialog::endSampleRecording);

    /// Dialog buttons
    /// ---

    /// Cancel button
    m_cancelButton = new QPushButton("Cancel", this);
    thirdLineLayout->addWidget(m_cancelButton);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    /// Calibrate button
    m_calibrateButton = new QPushButton("Calibrate", this);
    thirdLineLayout->addWidget(m_calibrateButton);
    m_calibrateButton->setEnabled(false);
    connect(m_calibrateButton, &QPushButton::clicked, this, &CalibrationDialog::calibrate);
    connect(m_table->model(), &QAbstractTableModel::rowsRemoved,
            [=](const QModelIndex &) { m_calibrateButton->setEnabled(m_table->rowCount() > 0); });
    connect(m_table->model(), &QAbstractTableModel::rowsInserted,
            [=](const QModelIndex &) { m_calibrateButton->setEnabled(true); });

    /// Save button
    m_saveButton = new QPushButton("Save", this);
    thirdLineLayout->addWidget(m_saveButton);
    m_saveButton->setEnabled(false);
}

void CalibrationDialog::handleCellActivation(int row, int column)
{
    using namespace qpmu;
    if (m_table->item(row, column)->flags() & Qt::ItemIsEnabled) {
        if (column == 0) {
            startSampleRecording(row);
        } else if (column == 1) {
            auto item = m_table->item(row, column);
            auto typeIndex = currentSignalTypeIndex();
            auto title = QStringLiteral("Enter actual magnitude");
            auto label = QStringLiteral("Enter the actual %1 value in %2 within the range [0, %3]")
                                 .arg(SignalTypeNames[typeIndex])
                                 .arg(SignalTypeUnitNames[typeIndex])
                                 .arg(m_magnitudeValidators[typeIndex]->top());
            auto validator = m_magnitudeValidators[typeIndex];
            bool ok;
            auto value = QInputDialog::getDouble(this, title, label, 0.0, 0.0, validator->top(),
                                                 validator->decimals(), &ok);
            if (!ok) {
                value = item->text().toDouble();
            }
            item->setText(QString::number(value, 'f', 2));
        }
    }
}

CalibrationDialog::USize CalibrationDialog::currentSignalIndex() const
{
    return m_signalChoiceButtonGroup->checkedId();
}

CalibrationDialog::USize CalibrationDialog::currentSignalTypeIndex() const
{
    return static_cast<USize>(qpmu::Signals[currentSignalIndex()].type);
}

void CalibrationDialog::calibrate()
{
    // TODO: Implement calibration
    if (m_doneCalibrating) {
        m_saveButton->setEnabled(true);
    }
}

void CalibrationDialog::addRow()
{
    using namespace qpmu;

    /// Add row
    const auto row = m_table->rowCount();
    m_table->insertRow(row);

    /// Sample magnitude cell
    auto sampleItem = new QTableWidgetItem();
    m_table->setItem(row, 0, sampleItem);
    sampleItem->setFlags(Qt::NoItemFlags | Qt::ItemIsEnabled);
    sampleItem->setTextAlignment(Qt::AlignRight);
    sampleItem->setIcon(QIcon(QStringLiteral(":/reload.png")));

    /// Actual magnitude cell
    auto actualItem = new QTableWidgetItem();
    m_table->setItem(row, 1, actualItem);
    actualItem->setFlags(Qt::NoItemFlags | Qt::ItemIsEnabled);
    actualItem->setTextAlignment(Qt::AlignRight);
    actualItem->setText(QStringLiteral("0.00"));
    actualItem->setToolTip(QStringLiteral("Enter the actual magnitude of the sample"));

    /// Delete button
    auto deleteButton = new QPushButton(m_table);
    m_table->setCellWidget(row, 2, deleteButton);
    deleteButton->setIcon(QIcon(QStringLiteral(":/delete.png")));
    deleteButton->setContentsMargins(QMargins(5, 0, 5, 0));
    connect(deleteButton, &QPushButton::clicked, [=] { deleteRow(sampleItem->row()); });

    startSampleRecording(sampleItem->row());
}

void CalibrationDialog::deleteRow(int row)
{
    if (m_recordedIntoItem && m_recordedIntoItem->row() == row) {
        endSampleRecording();
    }
    m_table->removeRow(row);
}

void CalibrationDialog::startSampleRecording(int row)
{
    m_addRowButton->setEnabled(false);
    for (int i = 0; i < m_table->rowCount(); ++i) {
        auto item = m_table->item(i, 0);
        item->setFlags(item->flags() & ~(Qt::ItemIsEnabled));
    }

    m_recordedIntoItem = m_table->item(row, 0);
    m_recordedIntoItem->setText(QStringLiteral("..."));

    m_recordingStartTime = QDateTime::currentMSecsSinceEpoch();
    m_valuesRead = 0;
    m_bufIdx = 0;
    m_buffer.fill(0);
    m_localMaxSum = 0;
    m_localMaxCount = 0;

    connect(APP->router(), &Router::newSampleObtained, this, &CalibrationDialog::processSample);
    m_recordingEndNotifier->start();
}

void CalibrationDialog::endSampleRecording()
{
    m_recordingEndNotifier->stop();
    disconnect(APP->router(), &Router::newSampleObtained, this, &CalibrationDialog::processSample);

    const auto avg = m_localMaxCount ? ((Float)m_localMaxSum / m_localMaxCount) : (Float)0;
    Q_ASSERT(m_recordedIntoItem);
    m_recordedIntoItem->setText(QString::number(avg, 'f', 2));
    m_recordedIntoItem = nullptr;

    m_addRowButton->setEnabled(true);
    for (int i = 0; i < m_table->rowCount(); ++i) {
        auto item = m_table->item(i, 0);
        item->setFlags(item->flags() | Qt::ItemIsEnabled);
    }
}

void CalibrationDialog::processSample(const qpmu::Sample &sample)
{
    const auto &value = sample.channel[currentSignalIndex()];
    m_buffer[m_bufIdx] = value;
    ++m_valuesRead;
    if (m_valuesRead >= BufferSize) {
        /// Test if the center is the local maximum
        bool isLocalMax = true;
        static constexpr auto Half = BufferSize / 2; /// BufferSize is odd
        /// if `m_bufIdx` is the last element, the center is `(m_bufIdx + Half + 1) % BufferSize`
        USize prvIdx = (m_bufIdx + Half + 1) % BufferSize;
        USize nxtIdx = (prvIdx + 1) % BufferSize;
        for (USize i = 0; i < Half; ++i) {
            if (!(m_buffer[prvIdx] >= m_buffer[nxtIdx])) {
                isLocalMax = false;
                break;
            }
            prvIdx = (prvIdx + 1) % BufferSize;
            nxtIdx = (nxtIdx + 1) % BufferSize;
        }
        if (isLocalMax) {
            prvIdx = (prvIdx + 1) % BufferSize;
            nxtIdx = (nxtIdx + 1) % BufferSize;
            for (USize i = 0; i < Half; ++i) {
                if (!(m_buffer[prvIdx] <= m_buffer[nxtIdx])) {
                    isLocalMax = false;
                    break;
                }
                prvIdx = (prvIdx + 1) % BufferSize;
                nxtIdx = (nxtIdx + 1) % BufferSize;
            }
            if (isLocalMax) {
                m_localMaxSum += value;
                m_localMaxCount += 1;
            }
        }
    }
    m_bufIdx = (m_bufIdx + 1) % BufferSize;
}
