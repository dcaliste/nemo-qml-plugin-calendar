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

CalendarEvent::Recur CalendarUtils::convertRecurrence(const KCalendarCore::Incidence::Ptr &event)
{
    if (!event->recurs())
        return CalendarEvent::RecurOnce;

    if (event->recurrence()->rRules().count() != 1)
        return CalendarEvent::RecurCustom;

    ushort rt = event->recurrence()->recurrenceType();
    int freq = event->recurrence()->frequency();

    if (rt == KCalendarCore::Recurrence::rDaily && freq == 1) {
        return CalendarEvent::RecurDaily;
    } else if (rt == KCalendarCore::Recurrence::rWeekly && freq == 1) {
        if (event->recurrence()->days().count(true) == 0) {
            return CalendarEvent::RecurWeekly;
        } else {
            return CalendarEvent::RecurWeeklyByDays;
        }
    } else if (rt == KCalendarCore::Recurrence::rWeekly && freq == 2 && event->recurrence()->days().count(true) == 0) {
        return CalendarEvent::RecurBiweekly;
    } else if (rt == KCalendarCore::Recurrence::rMonthlyDay && freq == 1) {
        return CalendarEvent::RecurMonthly;
    } else if (rt == KCalendarCore::Recurrence::rMonthlyPos && freq == 1) {
        const QList<KCalendarCore::RecurrenceRule::WDayPos> monthPositions = event->recurrence()->monthPositions();
        if (monthPositions.length() == 1
            && monthPositions.first().day() == event->dtStart().date().dayOfWeek()) {
            if (monthPositions.first().pos() > 0) {
                return CalendarEvent::RecurMonthlyByDayOfWeek;
            } else if (monthPositions.first().pos() == -1) {
                return CalendarEvent::RecurMonthlyByLastDayOfWeek;
            }
        }
    } else if (rt == KCalendarCore::Recurrence::rYearlyMonth && freq == 1) {
        return CalendarEvent::RecurYearly;
    }

    return CalendarEvent::RecurCustom;
}

CalendarEvent::Days CalendarUtils::convertDayPositions(const KCalendarCore::Incidence::Ptr &event)
{
    if (!event->recurs())
        return CalendarEvent::NoDays;

    if (event->recurrence()->rRules().count() != 1)
        return CalendarEvent::NoDays;

    if (event->recurrence()->recurrenceType() != KCalendarCore::Recurrence::rWeekly
        || event->recurrence()->frequency() != 1)
        return CalendarEvent::NoDays;

    const CalendarEvent::Day week[7] = {CalendarEvent::Monday,
                                        CalendarEvent::Tuesday,
                                        CalendarEvent::Wednesday,
                                        CalendarEvent::Thursday,
                                        CalendarEvent::Friday,
                                        CalendarEvent::Saturday,
                                        CalendarEvent::Sunday};

    const QList<KCalendarCore::RecurrenceRule::WDayPos> monthPositions = event->recurrence()->monthPositions();
    CalendarEvent::Days days = CalendarEvent::NoDays;
    for (QList<KCalendarCore::RecurrenceRule::WDayPos>::ConstIterator it = monthPositions.constBegin();
         it != monthPositions.constEnd(); ++it) {
        days |= week[it->day() - 1];
    }
    return days;
}

CalendarEvent::Secrecy CalendarUtils::convertSecrecy(const KCalendarCore::Incidence::Ptr &event)
{
    KCalendarCore::Incidence::Secrecy s = event->secrecy();
    switch (s) {
    case KCalendarCore::Incidence::SecrecyPrivate:
        return CalendarEvent::SecrecyPrivate;
    case KCalendarCore::Incidence::SecrecyConfidential:
        return CalendarEvent::SecrecyConfidential;
    case KCalendarCore::Incidence::SecrecyPublic:
    default:
        return CalendarEvent::SecrecyPublic;
    }
}

CalendarEvent::Status CalendarUtils::convertStatus(const KCalendarCore::Incidence::Ptr &event)
{
    switch (event->status()) {
    case KCalendarCore::Incidence::StatusTentative:
        return CalendarEvent::StatusTentative;
    case KCalendarCore::Incidence::StatusConfirmed:
        return CalendarEvent::StatusConfirmed;
    case KCalendarCore::Incidence::StatusCanceled:
        return CalendarEvent::StatusCancelled;
    case KCalendarCore::Incidence::StatusNone:
    default:
        return CalendarEvent::StatusNone;
    }
}

CalendarEvent::SyncFailure CalendarUtils::convertSyncFailure(const KCalendarCore::Incidence::Ptr &event)
{
    const QString &syncFailure = event->customProperty("VOLATILE", "SYNC-FAILURE");
    if (syncFailure.compare("upload", Qt::CaseInsensitive) == 0) {
        return CalendarEvent::UploadFailure;
    } else if (syncFailure.compare("update", Qt::CaseInsensitive) == 0) {
        return CalendarEvent::UpdateFailure;
    } else if (syncFailure.compare("delete", Qt::CaseInsensitive) == 0) {
        return CalendarEvent::DeleteFailure;
    }
    return CalendarEvent::NoSyncFailure;
}

bool CalendarUtils::getResponse(const KCalendarCore::Incidence::Ptr &event, const QString &calendarEmail, CalendarEvent::Response *response)
{
    // It would be good to set the attendance status directly in the event within the plugin,
    // however in some cases the account email and owner attendee email won't necessarily match
    // (e.g. in the case where server-side aliases are defined but unknown to the plugin).
    // So we handle this here to avoid "missing" some status changes due to owner email mismatch.
    // This defaults to QString() -> ResponseUnspecified in case the property is undefined
    if (response)
        *response = CalendarUtils::convertResponseType(event->nonKDECustomProperty("X-EAS-RESPONSE-TYPE"));
    const KCalendarCore::Attendee::List attendees = event->attendees();
    for (const KCalendarCore::Attendee &calAttendee : attendees) {
        if (calAttendee.email() == calendarEmail) {
            if (response
                && CalendarUtils::convertPartStat(calAttendee.status()) != CalendarEvent::ResponseUnspecified) {
                // Override the ResponseType
                *response = CalendarUtils::convertPartStat(calAttendee.status());
            }
            //TODO: KCalendarCore::Attendee::RSVP() returns false even if response was requested for some accounts like Google.
            // We can use attendee role until the problem is not fixed (probably in Google plugin).
            // To be updated later when google account support for responses is added.
            return calAttendee.RSVP();// || calAttendee->role() != KCalendarCore::Attendee::Chair;
        }
    }
    return false;
}

bool CalendarUtils::getExternalInvitation(const QString &organizerEmail, const CalendarData::Notebook &notebook)
{
    return !organizerEmail.isEmpty() && organizerEmail != notebook.emailAddress
            && !notebook.sharedWith.contains(organizerEmail);
}

int CalendarUtils::getReminder(const KCalendarCore::Incidence::Ptr &event)
{
    KCalendarCore::Alarm::List alarms = event->alarms();

    KCalendarCore::Alarm::Ptr alarm;

    int seconds = -1; // Any negative values means "no reminder"
    for (int ii = 0; ii < alarms.count(); ++ii) {
        if (alarms.at(ii)->type() == KCalendarCore::Alarm::Procedure)
            continue;
        alarm = alarms.at(ii);
        if (alarm && !alarm->hasTime()) {
            KCalendarCore::Duration d = alarm->startOffset();
            seconds = d.asSeconds() * -1; // backend stores as "offset in seconds to dtStart", we return "seconds before"
            if (seconds >= 0) {
                break;
            }
        }
        break;
    }

    return seconds;
}

QDateTime CalendarUtils::getReminderDateTime(const KCalendarCore::Incidence::Ptr &event)
{
    for (const KCalendarCore::Alarm::Ptr &alarm : event->alarms()) {
        if (alarm && alarm->type() == KCalendarCore::Alarm::Display && alarm->hasTime()) {
            return alarm->time();
        }
    }

    return QDateTime();
}

QList<CalendarData::Attendee> CalendarUtils::getEventAttendees(const KCalendarCore::Incidence::Ptr &event)
{
    QList<CalendarData::Attendee> result;
    const KCalendarCore::Person calOrganizer = event->organizer();

    CalendarData::Attendee organizer;
    if (!calOrganizer.email().isEmpty()) {
        organizer.isOrganizer = true;
        organizer.name = calOrganizer.name();
        organizer.email = calOrganizer.email();
        organizer.participationRole = KCalendarCore::Attendee::Chair;
        result.append(organizer);
    }

    const KCalendarCore::Attendee::List attendees = event->attendees();
    CalendarData::Attendee attendee;
    attendee.isOrganizer = false;

    for (const KCalendarCore::Attendee &calAttendee : attendees) {
        attendee.name = calAttendee.name();
        attendee.email = calAttendee.email();
        if (attendee.name == organizer.name && attendee.email == organizer.email) {
            // avoid duplicate info
            continue;
        }

        attendee.status = calAttendee.status();
        attendee.participationRole = calAttendee.role();
        result.append(attendee);
    }

    return result;
}

QList<QObject *> CalendarUtils::convertAttendeeList(const QList<CalendarData::Attendee> &list)
{
    QList<QObject*> result;
    foreach (const CalendarData::Attendee &attendee, list) {
        Person::AttendeeRole role;
        switch (attendee.participationRole) {
        case KCalendarCore::Attendee::ReqParticipant:
            role = Person::RequiredParticipant;
            break;
        case KCalendarCore::Attendee::OptParticipant:
            role = Person::OptionalParticipant;
            break;
        case KCalendarCore::Attendee::Chair:
            role = Person::ChairParticipant;
            break;
        default:
            role = Person::NonParticipant;
            break;
        }

        Person::ParticipationStatus status;
        switch (attendee.status) {
        case KCalendarCore::Attendee::Accepted:
            status = Person::AcceptedParticipation;
            break;
        case KCalendarCore::Attendee::Declined:
            status = Person::DeclinedParticipation;
            break;
        case KCalendarCore::Attendee::Tentative:
            status = Person::TentativeParticipation;
            break;
        default:
            status = Person::UnknownParticipation;
        }

        QObject *person = new Person(attendee.name, attendee.email, attendee.isOrganizer, role, status);
        result.append(person);
    }

    return result;
}

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

CalendarEvent::Response CalendarUtils::convertPartStat(KCalendarCore::Attendee::PartStat status)
{
    switch (status) {
    case KCalendarCore::Attendee::Accepted:
        return CalendarEvent::ResponseAccept;
    case KCalendarCore::Attendee::Declined:
        return CalendarEvent::ResponseDecline;
    case KCalendarCore::Attendee::Tentative:
        return CalendarEvent::ResponseTentative;
    case KCalendarCore::Attendee::NeedsAction:
    case KCalendarCore::Attendee::None:
    default:
        return CalendarEvent::ResponseUnspecified;
    }
}

KCalendarCore::Attendee::PartStat CalendarUtils::convertResponse(CalendarEvent::Response response)
{
    switch (response) {
    case CalendarEvent::ResponseAccept:
        return KCalendarCore::Attendee::Accepted;
    case CalendarEvent::ResponseTentative:
        return KCalendarCore::Attendee::Tentative;
    case CalendarEvent::ResponseDecline:
        return KCalendarCore::Attendee::Declined;
    default:
        return KCalendarCore::Attendee::NeedsAction;
    }
}

CalendarEvent::Response CalendarUtils::convertResponseType(const QString &responseType)
{
    // QString::toInt() conversion defaults to 0 on failure
    switch (responseType.toInt()) {
    case 1: // OrganizerResponseType (Organizer's acceptance is implicit)
    case 3: // AcceptedResponseType
        return CalendarEvent::ResponseAccept;
    case 2: // TentativeResponseType
        return CalendarEvent::ResponseTentative;
    case 4: // DeclinedResponseType
        return CalendarEvent::ResponseDecline;
    case -1: // ResponseTypeUnset
    case 0: // NoneResponseType
    case 5: // NotRespondedResponseType
    default:
        return CalendarEvent::ResponseUnspecified;
    }
}

QString CalendarUtils::recurrenceIdToString(const QDateTime &dt)
{
    // Convert to Qt::OffsetFromUTC spec to ensure time zone offset is included in string format,
    // to be consistent with previous versions that used KDateTime::toString() to produce the
    // same string format for recurrence ids.
    return dt.toOffsetFromUtc(dt.offsetFromUtc()).toString(Qt::ISODate);
}
