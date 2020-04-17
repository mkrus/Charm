/*
  TimeularSetupDialog.h

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2014-2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

  Author: Mike Krus <mike.krus@kdab.com>

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

#ifndef TIMEULARSETUPDIALOG_H
#define TIMEULARSETUPDIALOG_H

#include <QDialog>
#include <QScopedPointer>

#include "TimeularAdaptor.h"

class QLabel;
namespace Ui {
class TimeularSetupDialog;
}

class TimeularSetupDialog : public QDialog
{
    Q_OBJECT
public:
    TimeularSetupDialog(QVector<TimeularAdaptor::FaceMapping> mappings, TimeularManager *manager,
                        QWidget *parent = nullptr);
    ~TimeularSetupDialog();

    QVector<TimeularAdaptor::FaceMapping> mappings();

private:
    void selectTask();
    void clearFace();
    void faceChanged(TimeularManager::Orientation orientation);
    void discoveredDevicesChanged(const QStringList &dl);
    void deviceSelectionChanged();
    void setPairedDevice();
    void connectToDevice();
    void statusChanged(TimeularManager::Status status);

    QScopedPointer<Ui::TimeularSetupDialog> m_ui;
    QVector<TimeularAdaptor::FaceMapping> m_mappings;
    TimeularManager *m_manager = nullptr;
    QVector<QLabel *> m_labels;
};

#endif // TIMEULARSETUPDIALOG_H
