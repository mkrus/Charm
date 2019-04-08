/*
  SelectTaskDialog.h

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2014-2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

  Author: Frank Osterfeld <frank.osterfeld@kdab.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SELECTTASKDIALOG_H
#define SELECTTASKDIALOG_H

#include <Core/Task.h>

#include <QDialog>
#include <QScopedPointer>

namespace Ui {
class SelectTaskDialog;
}

class SelectTaskDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SelectTaskDialog(QWidget *parent = nullptr);
    ~SelectTaskDialog() override;

    TaskId selectedTask() const;
    void setNonValidSelectable();

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private Q_SLOTS:
    void slotAccepted();
    void slotResetState();
    void slotTaskSelected(TaskId id);
    void slotTaskDoubleClicked(TaskId id);

private:
    bool isValid(TaskId id) const;

private:
    QScopedPointer<Ui::SelectTaskDialog> m_ui;
    bool m_nonValidSelectable = false;
};

#endif
