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

static bool taskIdLessThan(const Task &left, const Task &right) {
    return left.id < right.id;
};

void TaskModel::setTasks(const TaskList &tasks)
{
    Q_ASSERT(!tasks.isEmpty());

    beginResetModel();

    m_tasks = tasks;
    m_items.clear();
    std::sort(m_tasks.begin(), m_tasks.end(), taskIdLessThan);

    const auto maxId = m_tasks.last().id;
    // Copmute how many digits do we need to display all task ids
    // For example, should return 4 if bigId is 9999.
    m_idPadding = static_cast<int>(ceil(log10(static_cast<double>(maxId))));

    computeTree(maxId);

    endResetModel();
}

void TaskModel::clearTasks()
{
    beginResetModel();

    m_tasks = {};
    m_items.clear();

    endResetModel();
}

int TaskModel::rowCount(const QModelIndex &parent) const
{
    return treeItemForIndex(parent).children.size();
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
        if (!task.isValid()) {
            QColor color("crimson");
            color.setAlphaF(0.25);
            return color;
        }
        return {};
    case Qt::DisplayRole:
        return QStringLiteral("%1 %2")
            .arg(task.id, m_idPadding, 10, QLatin1Char('0'))
            .arg(task.name);
    case TaskRole:
        return QVariant::fromValue(task);
    case IdRole:
        return QVariant::fromValue(task.id);
    case FilterRole:
        return QStringLiteral("%1 %2")
            .arg(task.id, m_idPadding, 10, QLatin1Char('0'))
            .arg(fullTaskName(task));
    }

    return {};
}

QModelIndex TaskModel::index(int row, int column, const QModelIndex &parent) const
{
    const auto &parentItem = treeItemForIndex(parent);
    return createIndex(row, column, static_cast<quintptr>(parentItem.children[row]));
}

QModelIndex TaskModel::parent(const QModelIndex &index) const
{
    const auto &item = treeItemForIndex(index);
    if (item.parentId == 0)
        return {};

    const auto &parentItem = m_items.at(item.parentId);
    const auto &grandParent = m_items.at(parentItem.parentId);
    const int row = grandParent.children.indexOf(parentItem.id);
    return createIndex(row, 0, static_cast<quintptr>(parentItem.id));
}

QModelIndex TaskModel::indexForTaskId(TaskId id) const
{
    Q_ASSERT(id >= 0 && id < m_items.size());
    return indexForTreeItem(m_items.at(id));
}

const Task &TaskModel::taskForId(TaskId id) const
{
    const auto &item = m_items.at(id);
    Q_ASSERT(item.id != 0);
    return *(item.task);
}

QString TaskModel::fullTaskName(const Task &task) const
{
    Q_ASSERT(!task.isNull());

    QString name = task.name.simplified();
    TaskId id = task.parent;

    while (id != 0) {
        const auto &item = m_items.at(id);
        name = item.task->name + QLatin1Char('/') + name;
        id = item.parentId;
    }
    return name;
}

TaskIdList TaskModel::childrenIds(const Task &task) const
{
    const auto &item = m_items.at(task.id);
    return item.children;
}

QModelIndex TaskModel::indexForTreeItem(const TreeItem &item) const
{
    if (item.id == 0)
        return {};

    const auto &parentItem = m_items.at(item.parentId);
    const int row = parentItem.children.indexOf(item.id);
    return createIndex(row, 0, static_cast<quintptr>(item.id));
}

const TaskModel::TreeItem &TaskModel::treeItemForIndex(const QModelIndex &index) const
{
    if (index.isValid())
        return m_items.at(static_cast<int>(index.internalId()));
    else
        return m_items[0];
}

const Task &TaskModel::taskForItem(const TreeItem &item) const
{
    return *(item.task);
}

void TaskModel::computeTree(int maxId)
{
    // We can build the tree in one pass, as all the item already exists
    // It doesn't mean that a child has an id bigger than its parent
    m_items.resize(maxId + 1);

    for (int i = 0; i < m_tasks.size(); ++i) {
        const auto &task = m_tasks.at(i);

        auto &item = m_items[task.id];
        auto &parent = m_items[task.parent];

        item.id = task.id;
        item.parentId = task.parent;
        item.task = &task;
        parent.children.push_back(task.id);
    }
}
