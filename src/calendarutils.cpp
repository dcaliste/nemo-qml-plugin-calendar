/*
 * Copyright (c) 2015-2019 Jolla Ltd.
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

#include "calendarutils.h"

#include "calendareventquery.h"

// kcalendarcore
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/VCalFormat>

//mkcal
#include <servicehandler.h>

// Qt
#include <QFile>
#include <QUrl>
#include <QString>
#include <QBitArray>
#include <QByteArray>
#include <QtDebug>

CalendarData::EventOccurrence CalendarUtils::getNextOccurrence(const KCalendarCore::Incidence::Ptr &incidence,
                                                               const QDateTime &start)
{
    const QTimeZone systemTimeZone = QTimeZone::systemTimeZone();

    CalendarData::EventOccurrence occurrence;
    if (incidence && incidence->type() == KCalendarCore::IncidenceBase::TypeEvent) {
        KCalendarCore::Event::Ptr event = incidence.staticCast<KCalendarCore::Event>();
        QDateTime dtStart = event->dtStart().toTimeZone(systemTimeZone);
        QDateTime dtEnd = event->dtEnd().toTimeZone(systemTimeZone);

        if (!start.isNull() && event->recurs()) {
            const QDateTime startTime = start.toTimeZone(systemTimeZone);
            KCalendarCore::Recurrence *recurrence = event->recurrence();
            if (recurrence->recursAt(startTime)) {
                dtStart = startTime;
                dtEnd = KCalendarCore::Duration(event->dtStart(), event->dtEnd()).end(startTime).toTimeZone(systemTimeZone);
            } else {
                QDateTime match = recurrence->getNextDateTime(startTime);
                if (match.isNull())
                    match = recurrence->getPreviousDateTime(startTime);

                if (!match.isNull()) {
                    dtStart = match.toTimeZone(systemTimeZone);
                    dtEnd = KCalendarCore::Duration(event->dtStart(), event->dtEnd()).end(match).toTimeZone(systemTimeZone);
                }
            }
        }

        occurrence.eventUid = event->uid();
        occurrence.recurrenceId = event->recurrenceId();
        occurrence.startTime = dtStart;
        occurrence.endTime = dtEnd;
        occurrence.eventAllDay = event->allDay();
    }

    return occurrence;
}

bool CalendarUtils::importFromFile(const QString &fileName,
                                   KCalendarCore::Calendar::Ptr calendar)
{
    QString filePath;
    QUrl url(fileName);
    if (url.isLocalFile())
        filePath = url.toLocalFile();
    else
        filePath = fileName;

    if (!(filePath.endsWith(".vcs") || filePath.endsWith(".ics"))) {
        qWarning() << "Unsupported file format" << filePath;
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Unable to open file for reading" << filePath;
        return false;
    }
    QByteArray fileContent(file.readAll());

    bool ok = false;
    if (filePath.endsWith(".vcs")) {
        KCalendarCore::VCalFormat vcalFormat;
        ok = vcalFormat.fromRawString(calendar, fileContent);
    } else if (filePath.endsWith(".ics")) {
        KCalendarCore::ICalFormat icalFormat;
        ok = icalFormat.fromRawString(calendar, fileContent);
    }
    if (!ok)
        qWarning() << "Failed to import from file" << filePath;

    return ok;
}

bool CalendarUtils::importFromIcsRawData(const QByteArray &icsData,
                                         KCalendarCore::Calendar::Ptr calendar)
{
    bool ok = false;
    KCalendarCore::ICalFormat icalFormat;
    ok = icalFormat.fromRawString(calendar, icsData);
    if (!ok)
        qWarning() << "Failed to import from raw data";

    return ok;
}

QString CalendarUtils::recurrenceIdToString(const QDateTime &dt)
{
    // Convert to Qt::OffsetFromUTC spec to ensure time zone offset is included in string format,
    // to be consistent with previous versions that used KDateTime::toString() to produce the
    // same string format for recurrence ids.
    return dt.toOffsetFromUtc(dt.offsetFromUtc()).toString(Qt::ISODate);
}
