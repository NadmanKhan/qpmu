#ifndef QPMU_APP_CALIBRATION_DIALOG_H
#define QPMU_APP_CALIBRATION_DIALOG_H

#include <QWidget>
#include <QList>
#include <QPair>
#include <QGridLayout>
#include <QButtonGroup>
#include <QTableWidget>
#include <QVariant>
#include <QDialog>
#include <QDoubleValidator>
#include <QTableWidget>
#include <QLabel>

#include "qpmu/common.h"
#include "timeout_notifier.h"

class CalibrationDialog : public QDialog
{
    Q_OBJECT

public:
    using Float = qpmu::Float;
    using U64 = qpmu::U64;
    using USize = qpmu::USize;
    static constexpr USize SecondsForSampleRecording = 3;
    static constexpr USize BufferSize = 5;
    static_assert(BufferSize % 2 == 1, "BufferSize must be odd");

    explicit CalibrationDialog(QWidget *parent = nullptr);
    USize currentSignalIndex() const;
    USize currentSignalTypeIndex() const;

private slots:
    void handleCellActivation(int row, int column);
    void addRow();
    void calibrate();
    void deleteRow(int row);
    void startSampleRecording(int row);
    void endSampleRecording();
    void processSample(const qpmu::Sample &sample);

private:
    QTableWidget *m_table = nullptr;
    QDoubleValidator *m_magnitudeValidators[qpmu::CountSignalTypes] = {};
    QButtonGroup *m_signalChoiceButtonGroup = nullptr;

    TimeoutNotifier *m_recordingEndNotifier = nullptr;
    QTableWidgetItem *m_recordedIntoItem = nullptr;
    quint64 m_recordingStartTime = 0;
    USize m_valuesRead = 0;
    USize m_bufIdx = 0;
    std::array<U64, BufferSize> m_buffer = {}; // circular buffer
    U64 m_localMaxSum = 0;
    U64 m_localMaxCount = 0;

    QPushButton *m_addRowButton = nullptr;
    QPushButton *m_cancelButton = nullptr;
    QPushButton *m_calibrateButton = nullptr;
    QPushButton *m_saveButton = nullptr;

    bool m_doneCalibrating = false;
    QPair<QLabel *, QLabel *> m_calibratedValues = {};
};

#endif // QPMU_APP_CALIBRATION_DIALOG_H