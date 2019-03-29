#ifndef TASKMODEL_H
#define TASKMODEL_H

#include "Task.h"

#include <QAbstractItemModel>
#include <QHash>
#include <QVector>

class TaskModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Roles {
        TaskRole = Qt::UserRole + 1,
        FilterRole,
    };

public:
    explicit TaskModel(QObject *parent = nullptr);

    const TaskList &tasks() const;
    void setTasks(const TaskList &tasks);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    QModelIndex indexForTaskId(TaskId id) const;
    Task taskForIndex(const QModelIndex &index) const;

    const Task &taskForId(TaskId id) const;
    QString fullTaskName(const Task &task) const;

private:
    struct TreeItem {
        TaskId id = 0;
        TreeItem *parent = nullptr;
        QVector<TreeItem *> children;
    };
    QModelIndex indexForTreeItem(TreeItem *item) const;
    const TreeItem *treeItemForIndex(const QModelIndex &index) const;
    const Task &taskForItem(const TreeItem *item) const;

private:
    TaskList m_tasks;
    QHash<TaskId, int> m_taskMap;

    // The first item is the hidden root item
    QVector<TreeItem> m_items;

    int m_idPadding = 4;
};

#endif // TASKMODEL_H
