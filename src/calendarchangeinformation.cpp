/*
 * Copyright (c) 2015 - 2019 Jolla Ltd.
 * Copyright (c) 2019 - 2020 Open Mobile Platform LLC.
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * "Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Nemo Mobile nor the names of its contributors
 *     may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 */

#include "calendarchangeinformation.h"
#include "calendarutils.h"

#include <QDebug>

CalendarChangeInformation::CalendarChangeInformation(QObject *parent) :
    QObject(parent), m_pending(true)
{
}

CalendarChangeInformation::~CalendarChangeInformation()
{
}

void CalendarChangeInformation::setInformation(const QString &uniqueId, const QDateTime &recurrenceId)
{
    m_uniqueId = uniqueId;
    m_recurrenceId = recurrenceId;
    m_pending = false;

    emit uniqueIdChanged();
    emit recurrenceIdChanged();
    emit pendingChanged();
}

bool CalendarChangeInformation::pending()
{
    return m_pending;
}

QString CalendarChangeInformation::uniqueId()
{
    return m_uniqueId;
}

QString CalendarChangeInformation::recurrenceId()
{
    return CalendarUtils::recurrenceIdToString(m_recurrenceId);
}
