#pragma once

#include <QAbstractItemModel>
#include <QVariant>
#include <functional>

/// Tree model (max depth 2) describing context menu controls declaratively.
/// Level 0 = sections, Level 1 = items (toggle, dropdown, slider, button).
/// Each item has a getter/setter lambda pair: the getter reads current state,
/// the setter applies user changes. Call refreshValues() to re-sync from getters.
class ContextItemModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum ContextItemRoles {
        TypeRole = Qt::UserRole + 1,
        LabelRole,
        ValueRole,
        OptionsRole,
        MinRole,
        MaxRole,
        StepRole,
        DecimalsRole,
        UnitRole,
        IconRole,
    };

    explicit ContextItemModel(QObject *parent = nullptr);

    // -- Builder API (returns item index) --
    int addSection(const QString &label);
    int addToggle(int sectionIdx, const QString &label,
                  const QStringList &options,
                  std::function<QVariant()> getter,
                  std::function<void(const QVariant &)> setter);
    int addDropdown(int sectionIdx, const QString &label,
                    const QStringList &options,
                    std::function<QVariant()> getter,
                    std::function<void(const QVariant &)> setter);
    int addSlider(int sectionIdx, const QString &label,
                  qreal min, qreal max, qreal step,
                  int decimals, const QString &unit,
                  std::function<QVariant()> getter,
                  std::function<void(const QVariant &)> setter);
    int addButton(int sectionIdx, const QString &label,
                  const QString &icon,
                  std::function<QVariant()> getter,
                  std::function<void(const QVariant &)> setter);

    void setValue(const QModelIndex &index, const QVariant &value);
    void refreshValues();

    // -- QAbstractItemModel interface --
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    struct Item {
        QString type;
        QString label;
        QVariant value;
        QStringList options;
        qreal min = 0, max = 0, step = 1;
        int decimals = 0;
        QString unit;
        QString icon;
        std::function<QVariant()> getter;
        std::function<void(const QVariant &)> setter;
        int parentIdx = -1;
        QList<int> children;
    };

    int addItem(Item item);
    const Item *itemAt(const QModelIndex &index) const;
    Item *mutableItemAt(const QModelIndex &index);

    QList<Item> m_items;
    QList<int> m_rootItems;
};
