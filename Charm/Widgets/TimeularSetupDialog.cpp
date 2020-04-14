/*
  TimeularAdaptor.cpp

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2014-2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

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

#include "TimeularSetupDialog.h"
#include "ui_TimeularSetupDialog.h"

#include "SelectTaskDialog.h"
#include "ViewHelpers.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStringListModel>
#include <QVBoxLayout>
#include <algorithm>

namespace {

QString labelText(TaskId taskId, int face, TimeularManager::Orientation activeFace)
{
    QString faceString = QString(QLatin1String(face == activeFace ? "<b>>%1</b>" : "<b>%1</b>")).arg(face);
    QString title = QString(QLatin1String("%1: Unassigned")).arg(faceString);
    if (taskId > 0)
        title = QString(QLatin1String("<b>%1</b>: %2")).arg(faceString).arg(DATAMODEL->smartTaskName(taskId));
    return title;
}

}
TimeularSetupDialog::TimeularSetupDialog(QVector<TimeularAdaptor::FaceMapping> mappings,
                                         TimeularManager *manager, QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::TimeularSetupDialog)
    , m_mappings(mappings)
    , m_manager(manager)
{
    m_ui->setupUi(this);
    m_ui->tabWidget->setCurrentIndex(0);

    // device selection
    // connect(m_ui->discoverButton)
    // task mapping
    QWidget *mappingWidgets = new QWidget(this);
    QVBoxLayout *l = new QVBoxLayout;

    for (int i=1; i<=8; i++) {
        QWidget *w = new QWidget(mappingWidgets);
        l->addWidget(w);

        auto taskId = -1;
        auto it = std::find_if(std::begin(m_mappings), std::end(m_mappings), [i](TimeularAdaptor::FaceMapping m) {
            return m.face == i;
        });
        if (it != m_mappings.end())
            taskId = (*it).taskId;

        QString title = labelText(taskId, i, m_manager->orientation());

        QLabel* label = new QLabel(title, w);
        label->setTextFormat(Qt::RichText);
        QPushButton *assignButton = new QPushButton(QLatin1String("Assign"), w);
        assignButton->setProperty("_timeularFaceIndex", i);
        connect(assignButton, &QPushButton::clicked, this, &TimeularSetupDialog::selectTask);
        QPushButton *clearButton = new QPushButton(QLatin1String("Clear"), w);
        clearButton->setProperty("_timeularFaceIndex", i);
        connect(clearButton, &QPushButton::clicked, this, &TimeularSetupDialog::clearFace);

        QHBoxLayout *h = new QHBoxLayout(w);
        h->setMargin(0);
        h->addWidget(label, 1);
        h->addWidget(assignButton);
        h->addWidget(clearButton);

        m_labels << label;
    }

    l->addStretch();
    mappingWidgets->setLayout(l);
    m_ui->scrollArea->setWidget(mappingWidgets);

    connect(m_manager, &TimeularManager::orientationChanged, this,
            &TimeularSetupDialog::faceChanged);

    QStringListModel *model = new QStringListModel(this);
    if (m_manager->isPaired()) {
        QStringList sl = {m_manager->pairedDevice() + QLatin1String(" - Paired")};
        model->setStringList(sl);
    }
    m_ui->listView->setModel(model);
    connect(m_manager, &TimeularManager::discoveredDevicesChanged, this,
            &TimeularSetupDialog::discoveredDevicesChanged);
    connect(m_manager, &TimeularManager::statusChanged, this, &TimeularSetupDialog::statusChanged);
    statusChanged(m_manager->status());

    //Discovery
    m_ui->discoverButton->setEnabled(m_manager->status() == TimeularManager::Disconneted);
    connect(m_ui->discoverButton, &QPushButton::clicked, this, [this] {
        auto status = m_manager->status();
        if (status == TimeularManager::Disconneted) {
            m_manager->startDiscovery();
        } else if (status == TimeularManager::Scanning) {
            m_manager->stopDiscovery();
        }
    });
    connect(m_manager, &TimeularManager::pairedChanged, this, [this]() {
        discoveredDevicesChanged(m_manager->discoveredDevices());
    });

    //Pairing
    m_ui->pairButton->setEnabled(false);
    connect(m_ui->listView->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &TimeularSetupDialog::deviceSelectionChanged);
    connect(m_ui->pairButton, &QPushButton::clicked, this, &TimeularSetupDialog::setPairedDevice);

    //Connection
    m_ui->connectButton->setEnabled(m_manager->isPaired());
    connect(m_ui->connectButton, &QPushButton::clicked, this, [this] {
        auto status = m_manager->status();
        if (status == TimeularManager::Disconneted) {
            m_manager->startConnection();
        } else if (status == TimeularManager::Connecting || status == TimeularManager::Connected) {
            m_manager->disconnect();
        }
    });
    connect(m_manager, &TimeularManager::pairedChanged, m_ui->connectButton,
            &QPushButton::setEnabled);
}

TimeularSetupDialog::~TimeularSetupDialog()
{
    if (m_manager->status() != TimeularManager::Connected)
        m_manager->disconnect();
}

QVector<TimeularAdaptor::FaceMapping> TimeularSetupDialog::mappings()
{
    return m_mappings;
}

void TimeularSetupDialog::selectTask()
{
    int faceId = sender()->property("_timeularFaceIndex").toInt();
    if (faceId < 1 || faceId > 8)
        return;
    auto it = std::find_if(std::begin(m_mappings), std::end(m_mappings), [faceId](const TimeularAdaptor::FaceMapping &m) {
        return m.face == faceId;
    });
    if (it == m_mappings.end()) {
        TimeularAdaptor::FaceMapping fm;
        fm.face = faceId;
        fm.taskId = -1;
        m_mappings << fm;
        it = std::find_if(std::begin(m_mappings), std::end(m_mappings), [faceId](const TimeularAdaptor::FaceMapping &m) {
            return m.face == faceId;
        });
    }

    SelectTaskDialog dialog(this);
    if (!dialog.exec())
        return;
    (*it).taskId = dialog.selectedTask();
    m_labels[faceId - 1]->setText(labelText((*it).taskId, faceId, m_manager->orientation()));
}

void TimeularSetupDialog::clearFace()
{
    int faceId = sender()->property("_timeularFaceIndex").toInt();
    if (faceId < 1 || faceId > 8)
        return;

    auto it = std::find_if(std::begin(m_mappings), std::end(m_mappings), [faceId](const TimeularAdaptor::FaceMapping &m) {
        return m.face == faceId;
    });
    m_mappings.erase(it);
    m_labels[faceId - 1]->setText( labelText(-1, faceId, m_manager->orientation()) );
}

void TimeularSetupDialog::faceChanged(TimeularManager::Orientation orientation)
{
    for (int i = 1; i < 9; i++) {
        auto taskId = -1;
        auto it = std::find_if(std::begin(m_mappings), std::end(m_mappings), [i](TimeularAdaptor::FaceMapping m) {
            return m.face == i;
        });
        if (it != m_mappings.end())
            taskId = (*it).taskId;
        m_labels[i - 1]->setText(labelText(taskId, i, orientation));
    }
}

void TimeularSetupDialog::discoveredDevicesChanged(QStringList dl)
{
    QStringListModel *model = qobject_cast<QStringListModel *>(m_ui->listView->model());
    QStringList fl;
    std::transform(dl.cbegin(), dl.cend(), std::back_inserter(fl), [this](QString deviceId) {
        return deviceId
            + (m_manager->pairedDevice() == deviceId ? QLatin1String(" - Paired") : QLatin1String(""));
    });
    model->setStringList(fl);
}

void TimeularSetupDialog::deviceSelectionChanged()
{
    auto sr = m_ui->listView->selectionModel()->selectedRows();
    if (sr.size() == 1) {
        int r = sr.front().row();
        if (r < m_manager->discoveredDevices().size() && m_manager->status() == TimeularManager::Disconneted) {
            m_ui->pairButton->setEnabled(m_manager->pairedDevice() != m_manager->discoveredDevices().at(r));
            return;
        }
    }

    m_ui->pairButton->setEnabled(false);
}

void TimeularSetupDialog::setPairedDevice()
{
    auto sr = m_ui->listView->selectionModel()->selectedRows();
    if (sr.size() != 1)
        return;

    int r = sr.front().row();
    if (r < m_manager->discoveredDevices().size())
        m_manager->setPairedDevice(m_manager->discoveredDevices().at(r));
}

void TimeularSetupDialog::connectToDevice()
{
    if (m_manager->isPaired())
        m_manager->startConnection();
}

void TimeularSetupDialog::statusChanged(TimeularManager::Status status)
{
    m_ui->discoverButton->setText(QLatin1Literal("Discover"));
    m_ui->connectButton->setText(QLatin1Literal("Connect"));

    if (status == TimeularManager::Disconneted) {
        m_ui->discoverButton->setEnabled(true);
        m_ui->connectButton->setEnabled(m_manager->isPaired());
    } else if (status == TimeularManager::Scanning) {
        m_ui->discoverButton->setText(QLatin1Literal("Stop Discovery"));
        m_ui->discoverButton->setEnabled(true);
        m_ui->connectButton->setEnabled(false);
    } else if (status == TimeularManager::Connecting) {
        m_ui->connectButton->setText(QLatin1Literal("Cancel Connection"));
        m_ui->discoverButton->setEnabled(false);
        m_ui->connectButton->setEnabled(true);
    } else if (status == TimeularManager::Connected) {
        m_ui->connectButton->setText(QLatin1Literal("Disconnect"));
        m_ui->discoverButton->setEnabled(false);
        m_ui->connectButton->setEnabled(true);
    } else {
        m_ui->discoverButton->setEnabled(false);
        m_ui->connectButton->setEnabled(false);
    }
}
