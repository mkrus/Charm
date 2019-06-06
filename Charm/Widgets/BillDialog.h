/*
  BillDialog.h

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

#ifndef BillDialog_H
#define BillDialog_H

#include <QDialog>

class BillDialog : public QDialog
{
    Q_OBJECT
public:
    enum BillResponse {
        Later,
        AsYouWish,
        AlreadyDone,
        AlreadyDoneAll,
    };
    explicit BillDialog(QWidget *parent = nullptr, Qt::WindowFlags f = {});
    void setReport(int year, int week);
    int year() const;
    int week() const;
private Q_SLOTS:
    void slotAsYouWish();
    void slotAlreadyDone();
    void slotLater();

private:
    QPushButton *m_asYouWish = nullptr;
    QPushButton *m_alreadyDone = nullptr;
    QPushButton *m_later = nullptr;
    QAction *m_alreadyDoneWeek = nullptr;
    int m_year = 0;
    int m_week = 0;
};

#endif
