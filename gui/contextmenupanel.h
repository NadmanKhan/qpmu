#pragma once

#include <QWidget>

class QScrollArea;
class QPropertyAnimation;
class QVBoxLayout;
class ContextItemModel;

class ContextMenuPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ContextMenuPanel(QWidget *parent = nullptr);

    void setModel(ContextItemModel *model);
    void toggle();
    void showPanel();
    void hidePanel();
    void updateGeometry(int parentWidth, int parentHeight, int topOffset);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void rebuildContent();
    void syncFromModel(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                       const QList<int> &roles);

    static constexpr int PANEL_WIDTH = 300;
    static constexpr int ANIM_DURATION = 200;

    ContextItemModel *m_model = nullptr;
    QPropertyAnimation *m_animation;
    QScrollArea *m_scrollArea;
    QWidget *m_contentWidget = nullptr;

    int m_topOffset = 0;
    int m_parentWidth = 0;
    int m_parentHeight = 0;

    // Maps model internal-id → widget for syncing values
    QHash<int, QWidget *> m_controlMap;
};
