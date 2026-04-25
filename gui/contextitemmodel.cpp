#include "contextitemmodel.h"

// ── Construction ────────────────────────────────────────────────────────────

ContextItemModel::ContextItemModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

// ── Builder API ─────────────────────────────────────────────────────────────

int ContextItemModel::addItem(Item item)
{
    int idx = m_items.size();
    if (item.parentIdx < 0) {
        m_rootItems.append(idx);
    } else {
        m_items[item.parentIdx].children.append(idx);
    }
    m_items.append(std::move(item));
    return idx;
}

int ContextItemModel::addSection(const QString &label)
{
    return addItem({ .type = QStringLiteral("section"), .label = label });
}

int ContextItemModel::addToggle(int sectionIdx, const QString &label,
                                const QStringList &options,
                                std::function<QVariant()> getter,
                                std::function<void(const QVariant &)> setter)
{
    return addItem({ .type = QStringLiteral("toggle"),
                     .label = label,
                     .value = getter(),
                     .options = options,
                     .getter = std::move(getter),
                     .setter = std::move(setter),
                     .parentIdx = sectionIdx });
}

int ContextItemModel::addDropdown(int sectionIdx, const QString &label,
                                  const QStringList &options,
                                  std::function<QVariant()> getter,
                                  std::function<void(const QVariant &)> setter)
{
    return addItem({ .type = QStringLiteral("dropdown"),
                     .label = label,
                     .value = getter(),
                     .options = options,
                     .getter = std::move(getter),
                     .setter = std::move(setter),
                     .parentIdx = sectionIdx });
}

int ContextItemModel::addSlider(int sectionIdx, const QString &label,
                                qreal min, qreal max, qreal step,
                                int decimals, const QString &unit,
                                std::function<QVariant()> getter,
                                std::function<void(const QVariant &)> setter)
{
    return addItem({ .type = QStringLiteral("slider"),
                     .label = label,
                     .value = getter(),
                     .min = min,
                     .max = max,
                     .step = step,
                     .decimals = decimals,
                     .unit = unit,
                     .getter = std::move(getter),
                     .setter = std::move(setter),
                     .parentIdx = sectionIdx });
}

int ContextItemModel::addButton(int sectionIdx, const QString &label,
                                const QString &icon,
                                std::function<QVariant()> getter,
                                std::function<void(const QVariant &)> setter)
{
    return addItem({ .type = QStringLiteral("button"),
                     .label = label,
                     .icon = icon,
                     .getter = std::move(getter),
                     .setter = std::move(setter),
                     .parentIdx = sectionIdx });
}

// ── Value management ────────────────────────────────────────────────────────

void ContextItemModel::setValue(const QModelIndex &idx, const QVariant &value)
{
    auto *item = mutableItemAt(idx);
    if (!item || !item->setter)
        return;
    item->setter(value);
    refreshValues();
}

void ContextItemModel::refreshValues()
{
    for (int i = 0; i < m_items.size(); ++i) {
        auto &item = m_items[i];
        if (!item.getter)
            continue;
        QVariant newVal = item.getter();
        if (item.value != newVal) {
            item.value = newVal;
            int row = item.parentIdx < 0
                ? m_rootItems.indexOf(i)
                : m_items[item.parentIdx].children.indexOf(i);
            QModelIndex idx = createIndex(row, 0, quintptr(i));
            emit dataChanged(idx, idx, { ValueRole });
        }
    }
}

// ── QAbstractItemModel interface ────────────────────────────────────────────

QModelIndex ContextItemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (column != 0)
        return {};
    if (!parent.isValid()) {
        if (row < 0 || row >= m_rootItems.size())
            return {};
        return createIndex(row, 0, quintptr(m_rootItems[row]));
    }
    int parentItemIdx = int(parent.internalId());
    if (parentItemIdx < 0 || parentItemIdx >= m_items.size())
        return {};
    const auto &children = m_items[parentItemIdx].children;
    if (row < 0 || row >= children.size())
        return {};
    return createIndex(row, 0, quintptr(children[row]));
}

QModelIndex ContextItemModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return {};
    int itemIdx = int(child.internalId());
    if (itemIdx < 0 || itemIdx >= m_items.size())
        return {};
    int parentIdx = m_items[itemIdx].parentIdx;
    if (parentIdx < 0)
        return {};
    int row = m_items[parentIdx].parentIdx < 0
                  ? m_rootItems.indexOf(parentIdx)
                  : m_items[m_items[parentIdx].parentIdx].children.indexOf(parentIdx);
    return createIndex(row, 0, quintptr(parentIdx));
}

int ContextItemModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_rootItems.size();
    int itemIdx = int(parent.internalId());
    if (itemIdx < 0 || itemIdx >= m_items.size())
        return 0;
    return m_items[itemIdx].children.size();
}

int ContextItemModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant ContextItemModel::data(const QModelIndex &index, int role) const
{
    const auto *item = itemAt(index);
    if (!item)
        return {};
    switch (role) {
    case TypeRole:     return item->type;
    case LabelRole:    return item->label;
    case ValueRole:    return item->value;
    case OptionsRole:  return item->options;
    case MinRole:      return item->min;
    case MaxRole:      return item->max;
    case StepRole:     return item->step;
    case DecimalsRole: return item->decimals;
    case UnitRole:     return item->unit;
    case IconRole:     return item->icon;
    default:           return {};
    }
}

QHash<int, QByteArray> ContextItemModel::roleNames() const
{
    return {
        { TypeRole,     "type"     },
        { LabelRole,    "label"    },
        { ValueRole,    "value"    },
        { OptionsRole,  "options"  },
        { MinRole,      "min"      },
        { MaxRole,      "max"      },
        { StepRole,     "step"     },
        { DecimalsRole, "decimals" },
        { UnitRole,     "unit"     },
        { IconRole,     "icon"     },
    };
}

// ── Item access ─────────────────────────────────────────────────────────────

const ContextItemModel::Item *ContextItemModel::itemAt(const QModelIndex &index) const
{
    if (!index.isValid())
        return nullptr;
    int idx = int(index.internalId());
    if (idx < 0 || idx >= m_items.size())
        return nullptr;
    return &m_items[idx];
}

ContextItemModel::Item *ContextItemModel::mutableItemAt(const QModelIndex &index)
{
    if (!index.isValid())
        return nullptr;
    int idx = int(index.internalId());
    if (idx < 0 || idx >= m_items.size())
        return nullptr;
    return &m_items[idx];
}
