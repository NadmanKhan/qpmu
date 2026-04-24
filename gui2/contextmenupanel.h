#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QComboBox;
class QSlider;
class QScrollArea;
class QPropertyAnimation;
class SignalDataModel;

class ContextMenuPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ContextMenuPanel(SignalDataModel *model, QWidget *parent = nullptr);

    void toggle();
    void showPanel();
    void hidePanel();
    void updateGeometry(int parentWidth, int parentHeight, int topOffset);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QWidget *buildContent();
    void syncFromModel();

    static constexpr int PANEL_WIDTH = 300;
    static constexpr int ANIM_DURATION = 200;

    SignalDataModel *m_model;
    QPropertyAnimation *m_animation;

    int m_topOffset = 0;
    int m_parentWidth = 0;
    int m_parentHeight = 0;

    // Data controls
    QPushButton *m_magnitudeToggle;
    QComboBox *m_phaseRefCombo;
    QPushButton *m_pauseButton;

    // Plot controls
    QPushButton *m_voltageScaleToggle;
    QSlider *m_voltageSlider;
    QLabel *m_voltageSliderValue;
    QWidget *m_voltageSliderRow;

    QPushButton *m_currentScaleToggle;
    QSlider *m_currentSlider;
    QLabel *m_currentSliderValue;
    QWidget *m_currentSliderRow;
};
