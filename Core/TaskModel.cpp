#include "TaskModel.h"

#include <QColor>
#include <QHash>

#include <algorithm>
#include <cmath>

TaskModel::TaskModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

const TaskList &TaskModel::tasks() const
{
    return m_tasks;
}

void TaskModel::setTasks(const TaskList &tasks)
{
    Q_ASSERT(!tasks.isEmpty());

    beginResetModel();

    m_tasks = tasks;
    m_taskMap.clear();
    m_items.clear();

    std::sort(m_tasks.begin(), m_tasks.end());
    const auto bigId = m_tasks.last().id();

    // Copmute how many digits do we need to display all task ids
    // For example, should return 4 if bigId is 9999.
    m_idPadding = static_cast<int>(ceil(log10(static_cast<double>(bigId))));

    // We assume that a task id is always lower than the ids of its children
    // This allow to construct the tree in one pass only
    m_items.resize(bigId + 1);
    m_taskMap[0] = 0;

    for (int i = 0; i < m_tasks.size(); ++i) {
        const auto &task = m_tasks.at(i);
        m_taskMap[task.id()] = i;

        auto &item = m_items[task.id()];
        auto &parent = m_items[task.parent()];
        item.id = task.id();
        item.parent = &parent;
        parent.children.push_back(&item);
    }

    endResetModel();
}

void TaskModel::clearTasks()
{
    beginResetModel();

    m_tasks = {};
    m_taskMap.clear();
    m_items.clear();

    endResetModel();
}

int TaskModel::rowCount(const QModelIndex &parent) const
{
    return treeItemForIndex(parent)->children.size();
}

int TaskModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QVariant TaskModel::data(const QModelIndex &index, int role) const
{
    Q_ASSERT(index.isValid());

    auto item = treeItemForIndex(index);
    const auto &task = taskForItem(item);

    switch (role) {
    case Qt::BackgroundRole:
        if (!task.isCurrentlyValid()) {
            QColor color("crimson");
            color.setAlphaF(0.25);
            return color;
        }
        return {};
    case Qt::DisplayRole:
        return QStringLiteral("%1 %2")
            .arg(task.id(), m_idPadding, 10, QLatin1Char('0'))
            .arg(task.name());
    case TaskRole:
        return QVariant::fromValue(task);
    case FilterRole:
        return QStringLiteral("%1 %2")
            .arg(task.id(), m_idPadding, 10, QLatin1Char('0'))
            .arg(fullTaskName(task));
    }

    return {};
}

QModelIndex TaskModel::index(int row, int column, const QModelIndex &parent) const
{
    auto parentItem = treeItemForIndex(parent);
    return createIndex(row, column, parentItem->children[row]);
}

QModelIndex TaskModel::parent(const QModelIndex &index) const
{
    auto item = treeItemForIndex(index);
    if (!item->parent)
        return {};
    auto parentItem = item->parent;
    if (!parentItem->parent)
        return {};
    const int row = parentItem->parent->children.indexOf(parentItem);
    return createIndex(row, 0, parentItem);
}

QModelIndex TaskModel::indexForTaskId(TaskId id) const
{
    Q_ASSERT(id >= 0 && id < m_items.size());
    return indexForTreeItem(const_cast<TreeItem *>(&m_items[id]));
}

Task TaskModel::taskForIndex(const QModelIndex &index) const
{
    Q_ASSERT(index.isValid());

    auto item = treeItemForIndex(index);
    auto it = std::find_if(m_tasks.cbegin(), m_tasks.cend(),
                           [id = item->id](const auto &task) { return task.id() == id; });
    if (it != m_tasks.cend())
        return *it;
    return {};
}

const Task &TaskModel::taskForId(TaskId id) const
{
    Q_ASSERT(m_taskMap.contains(id));
    return m_tasks.at(m_taskMap.value(id));
}

QString TaskModel::fullTaskName(const Task &task) const
{
    Q_ASSERT(task.isValid());
    auto item = &(m_items.at(task.id()));

    QString name = task.name().simplified();
    item = item->parent;

    while (item->parent != nullptr) {
        const auto &task = taskForItem(item);
        name = task.name().simplified() + QLatin1Char('/') + name;
        item = item->parent;
    }
    return name;
}

TaskIdList TaskModel::childrenIds(const Task &task) const
{
    auto item = m_items.at(task.id());

    TaskIdList ids;
    for (auto child : item.children)
        ids.push_back(child->id);

    return ids;
}

QModelIndex TaskModel::indexForTreeItem(TreeItem *item) const
{
    if (item->id == 0)
        return {};

    const int row = item->parent->children.indexOf(item);
    return createIndex(row, 0, item);
}

const TaskModel::TreeItem *TaskModel::treeItemForIndex(const QModelIndex &index) const
{
    if (index.isValid())
        return static_cast<TreeItem *>(index.internalPointer());
    else
        return &m_items[0];
}

const Task &TaskModel::taskForItem(const TaskModel::TreeItem *item) const
{
    return m_tasks.at(m_taskMap.value(item->id));
}
