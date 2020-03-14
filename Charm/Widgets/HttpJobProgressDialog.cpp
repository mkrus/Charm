/*
  HttpJobProgressDialog.cpp

  This file is part of Charm, a task-based time tracking application.

  Copyright (C) 2014-2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

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

#include "HttpJobProgressDialog.h"
#include <QInputDialog>
#include <QLineEdit>

HttpJobProgressDialog::HttpJobProgressDialog(HttpJob *job, QWidget *parent)
    : QProgressDialog(parent)
    , m_job(job)
{
    setLabelText(tr("Wait..."));

    Q_ASSERT(job);
    connect(job, &HttpJob::finished, this, &HttpJobProgressDialog::jobFinished);
    connect(job, &HttpJob::transferStarted, this, &HttpJobProgressDialog::jobTransferStarted);
    connect(job, &HttpJob::passwordRequested, this, &HttpJobProgressDialog::jobPasswordRequested);
}

void HttpJobProgressDialog::jobTransferStarted()
{
    show();
}

void HttpJobProgressDialog::jobFinished(HttpJob *)
{
    deleteLater();
}

void HttpJobProgressDialog::jobPasswordRequested(HttpJob::PasswordRequestReason reason)
{
    bool ok;
    QPointer<QObject> that(this); // guard against destruction while dialog is open

    const auto title =
        reason == HttpJob::PasswordIncorrect ? tr("Authentication Failed") : tr("Password");
    const auto message = reason == HttpJob::PasswordIncorrect
        ? tr("Please re-enter your lotsofcake password")
        : tr("Please enter your lotsofcake password");
    const auto newpass = QInputDialog::getText(parentWidget(), title, message, QLineEdit::Password,
                                               m_job->password(), &ok);

    if (!that)
        return;
    if (ok) {
        m_job->provideRequestedPassword(newpass);
    } else {
        m_job->passwordRequestCanceled();
    }
}
