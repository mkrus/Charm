/*
  CharmPreferences.cpp

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2014-2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

  Author: Frank Osterfeld <frank.osterfeld@kdab.com>
  Author: Mathias Hasselmann <mathias.hasselmann@kdab.com>

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

#include "CharmPreferences.h"
#include "ApplicationCore.h"

#include "Core/Configuration.h"
#include "Idle/IdleDetector.h"
#include "Lotsofcake/Configuration.h"

#include <QCheckBox>
#include <QMessageBox>

CharmPreferences::CharmPreferences(const Configuration &config, QWidget *parent_)
    : QDialog(parent_)
{
    m_ui.setupUi(this);
    const bool haveIdleDetection = ApplicationCore::instance().idleDetector()->available();
    const bool haveCommandInterface = (ApplicationCore::instance().commandInterface() != nullptr);
    const bool httpJobPossible = Lotsofcake::Configuration().isConfigured();

    m_ui.lbWarnUnuploadedTimesheets->setVisible(httpJobPossible);
    m_ui.cbWarnUnuploadedTimesheets->setVisible(httpJobPossible);
    m_ui.cbIdleDetection->setEnabled(haveIdleDetection);
    m_ui.lbIdleDetection->setEnabled(haveIdleDetection);
    m_ui.cbIdleDetection->setChecked(config.detectIdling && m_ui.cbIdleDetection->isEnabled());
    m_ui.cbWarnUnuploadedTimesheets->setChecked(config.warnUnuploadedTimesheets);
    m_ui.cbRequestEventComment->setChecked(config.requestEventComment);

    connect(m_ui.cbWarnUnuploadedTimesheets, &QCheckBox::toggled, this,
            &CharmPreferences::slotWarnUnuploadedChanged);

    // this would not need a switch, but i hate casting enums to int:
    switch (config.timeTrackerFontSize) {
    case Configuration::TimeTrackerFont_ExtraSmall:
        m_ui.cbTimeTrackerFontSize->setCurrentIndex(0);
        break;
    case Configuration::TimeTrackerFont_Small:
        m_ui.cbTimeTrackerFontSize->setCurrentIndex(1);
        break;
    case Configuration::TimeTrackerFont_Regular:
        m_ui.cbTimeTrackerFontSize->setCurrentIndex(2);
        break;
    case Configuration::TimeTrackerFont_Large:
        m_ui.cbTimeTrackerFontSize->setCurrentIndex(3);
        break;
    case Configuration::TimeTrackerFont_ExtraLarge:
        m_ui.cbTimeTrackerFontSize->setCurrentIndex(4);
        break;
    }

    switch (config.durationFormat) {
    case Configuration::Minutes:
        m_ui.cbDurationFormat->setCurrentIndex(0);
        break;
    case Configuration::Decimal:
        m_ui.cbDurationFormat->setCurrentIndex(1);
        break;
    }

    switch (config.toolButtonStyle) {
    case Qt::ToolButtonIconOnly:
        m_ui.cbToolButtonStyle->setCurrentIndex(0);
        break;
    case Qt::ToolButtonTextOnly:
        m_ui.cbToolButtonStyle->setCurrentIndex(1);
        break;
    case Qt::ToolButtonTextUnderIcon:
        m_ui.cbToolButtonStyle->setCurrentIndex(2);
        break;
    case Qt::ToolButtonTextBesideIcon:
        m_ui.cbToolButtonStyle->setCurrentIndex(3);
        break;
    case Qt::ToolButtonFollowStyle:
        m_ui.cbToolButtonStyle->setCurrentIndex(4);
        break;
    }

    m_ui.sbNumberOfTaskSelectorEntries->setValue(config.numberOfTaskSelectorEntries);

    // resize( minimumSize() );
}

CharmPreferences::~CharmPreferences() {}

bool CharmPreferences::detectIdling() const
{
    return m_ui.cbIdleDetection->isChecked();
}

bool CharmPreferences::warnUnuploadedTimesheets() const
{
    return m_ui.cbWarnUnuploadedTimesheets->isChecked();
}

bool CharmPreferences::requestEventComment() const
{
    return m_ui.cbRequestEventComment->isChecked();
}

int CharmPreferences::numberOfTaskSelectorEntries() const
{
    return m_ui.sbNumberOfTaskSelectorEntries->value();
}

Configuration::DurationFormat CharmPreferences::durationFormat() const
{
    switch (m_ui.cbDurationFormat->currentIndex()) {
    case 0:
        return Configuration::Minutes;
    case 1:
        return Configuration::Decimal;
    default:
        Q_ASSERT(!"Unexpected combobox item for DurationFormat");
    }
    return Configuration::Minutes;
}

Configuration::TimeTrackerFontSize CharmPreferences::timeTrackerFontSize() const
{
    switch (m_ui.cbTimeTrackerFontSize->currentIndex()) {
    case 0:
        return Configuration::TimeTrackerFont_ExtraSmall;
    case 1:
        return Configuration::TimeTrackerFont_Small;
    case 2:
        return Configuration::TimeTrackerFont_Regular;
    case 3:
        return Configuration::TimeTrackerFont_Large;
    case 4:
        return Configuration::TimeTrackerFont_ExtraLarge;
    default:
        Q_ASSERT(false); // somebody added an item
    }
    // always return something, to avoid compiler warning:
    return Configuration::TimeTrackerFont_Regular;
}

Qt::ToolButtonStyle CharmPreferences::toolButtonStyle() const
{
    switch (m_ui.cbToolButtonStyle->currentIndex()) {
    case 0:
        return Qt::ToolButtonIconOnly;
    case 1:
        return Qt::ToolButtonTextOnly;
    case 2:
        return Qt::ToolButtonTextUnderIcon;
    case 3:
        return Qt::ToolButtonTextBesideIcon;
    case 4:
        return Qt::ToolButtonFollowStyle;
    default:
        Q_ASSERT(false); // somebody added an item
    }
    // always return something, to avoid compiler warning:
    return Qt::ToolButtonIconOnly;
}

void CharmPreferences::slotWarnUnuploadedChanged(bool enabled)
{
    if (!Lotsofcake::Configuration().isConfigured())
        return;

    if (!enabled) {
        QMessageBox mbox(QMessageBox::Question, tr("Bill is sad :(."),
                         tr("Bill has always been misunderstood. All he really wants is your "
                            "reports, and even when he doesn't get them you only have to evade him "
                            "once per hour. I'm sure you want to keep Bill's gentle reminders?"),
                         QMessageBox::Yes | QMessageBox::No);
        auto buttonYes = mbox.button(QMessageBox::Yes);
        buttonYes->setText(tr("Mmmmkay"));
        auto buttonNo = mbox.button(QMessageBox::No);
        buttonNo->setText(tr("No, Stop Bill"));

        if (mbox.exec() == QMessageBox::Yes)
            m_ui.cbWarnUnuploadedTimesheets->setCheckState(Qt::Checked);
    }
}
