/*
 * Copyright (c) 2014 - 2019 Jolla Ltd.
 * Copyright (c) 2020 Open Mobile Platform LLC.
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

#include "calendareventmodification.h"
#include "calendarmanager.h"
#include "calendarutils.h"

#include <QTimeZone>
#include <QDebug>

namespace {

void updateTime(QDateTime *dt, Qt::TimeSpec spec, const QString &timeZone)
{
    if (spec == Qt::TimeZone) {
        QTimeZone tz(timeZone.toUtf8());
        if (tz.isValid()) {
            dt->setTimeZone(tz);
        } else {
            qWarning() << "Cannot find time zone:" << timeZone;
        }
    } else {
        dt->setTimeSpec(spec);
    }
}

}

CalendarEventModification::CalendarEventModification(const CalendarStoredEvent &data, QObject *parent)
    : CalendarEvent(data, parent)
    , m_attendeesSet(false)
{
}

CalendarEventModification::CalendarEventModification(QObject *parent)
    : CalendarEvent(parent)
    , m_attendeesSet(false)
{
}

CalendarEventModification::~CalendarEventModification()
{
}

void CalendarEventModification::setDisplayLabel(const QString &displayLabel)
{
    mIncidence.data->resetDirtyFields();
    mIncidence.data->setSummary(displayLabel);
    if (mIncidence.data->dirtyFields().contains(KCalendarCore::IncidenceBase::FieldSummary))
        emit displayLabelChanged();
}

void CalendarEventModification::setDescription(const QString &description)
{
    mIncidence.data->resetDirtyFields();
    mIncidence.data->setDescription(description);
    if (mIncidence.data->dirtyFields().contains(KCalendarCore::IncidenceBase::FieldDescription))
        emit descriptionChanged();
}

QDateTime CalendarEventModification::startTime() const
{
    return mIncidence.data->dtStart();
}

void CalendarEventModification::setStartTime(const QDateTime &startTime, Qt::TimeSpec spec, const QString &timezone)
{
    mIncidence.data->resetDirtyFields();
    QDateTime newStartTimeInTz = startTime;
    updateTime(&newStartTimeInTz, spec, timezone);
    mIncidence.data->setDtStart(newStartTimeInTz);
    if (mIncidence.data->dirtyFields().contains(KCalendarCore::IncidenceBase::FieldDtStart))
        emit startTimeChanged();
}

QDateTime CalendarEventModification::endTime() const
{
    return mIncidence.data->type() == KCalendarCore::IncidenceBase::TypeEvent ? mIncidence.data.staticCast<KCalendarCore::Event>()->dtEnd() : QDateTime();
}

void CalendarEventModification::setEndTime(const QDateTime &endTime, Qt::TimeSpec spec, const QString &timezone)
{
    mIncidence.data->resetDirtyFields();
    QDateTime newEndTimeInTz = endTime;
    updateTime(&newEndTimeInTz, spec, timezone);
    if (mIncidence.data->type() == KCalendarCore::IncidenceBase::TypeEvent)
        mIncidence.data.staticCast<KCalendarCore::Event>()->setDtEnd(newEndTimeInTz);
    if (mIncidence.data->dirtyFields().contains(KCalendarCore::IncidenceBase::FieldDtEnd))
        emit endTimeChanged();
}

void CalendarEventModification::setAllDay(bool allDay)
{
    if (mIncidence.data->allDay() != allDay) {
        mIncidence.data->setAllDay(allDay);
        emit allDayChanged();
    }
}

void CalendarEventModification::setRecur(CalendarEvent::Recur recur)
{
    if (mRecur != recur) {
        mRecur = recur;
        emit recurChanged();
    }
}

void CalendarEventModification::setRecurEndDate(const QDateTime &dateTime)
{
    bool wasValid = hasRecurEndDate();
    QDate date = dateTime.date();

    if (mRecurEndDate != date) {
        mRecurEndDate = date;
        emit recurEndDateChanged();

        if (date.isValid() != wasValid) {
            emit hasRecurEndDateChanged();
        }
    }
}

void CalendarEventModification::unsetRecurEndDate()
{
    setRecurEndDate(QDateTime());
}

void CalendarEventModification::setRecurWeeklyDays(CalendarEvent::Days days)
{
    if (mRecurWeeklyDays != days) {
        mRecurWeeklyDays = days;
        emit recurWeeklyDaysChanged();
    }
}

void CalendarEventModification::setReminder(int seconds)
{
    if (seconds != mReminder) {
        mReminder = seconds;
        emit reminderChanged();
    }
}

void CalendarEventModification::setReminderDateTime(const QDateTime &dateTime)
{
    if (dateTime != mReminderDateTime) {
        mReminderDateTime = dateTime;
        emit reminderDateTimeChanged();
    }
}

void CalendarEventModification::setLocation(const QString &newLocation)
{
    mIncidence.data->resetDirtyFields();
    mIncidence.data->setLocation(newLocation);
    if (mIncidence.data->dirtyFields().contains(KCalendarCore::IncidenceBase::FieldLocation))
        emit locationChanged();
}

void CalendarEventModification::setCalendarUid(const QString &uid)
{
    if (mIncidence.notebookUid != uid) {
        mIncidence.notebookUid = uid;
        emit calendarUidChanged();
    }
}

void CalendarEventModification::setAttendees(CalendarContactModel *required, CalendarContactModel *optional)
{
    if (!required || !optional) {
        qWarning() << "Missing attendeeList";
        return;
    }

    m_attendeesSet = true;
    m_requiredAttendees = required->getList();
    m_optionalAttendees = optional->getList();
}

static void updateAttendee(KCalendarCore::Attendee::List &attendees,
                           const KCalendarCore::Person &contact,
                           KCalendarCore::Attendee::Role role)
{
    KCalendarCore::Attendee::List::Iterator it =
        std::find_if(attendees.begin(), attendees.end(),
                     [contact] (const KCalendarCore::Attendee &attendee)
                     {return attendee.email() == contact.email();});
    if (it == attendees.end()) {
        attendees.append(KCalendarCore::Attendee
                         (contact.name(), contact.email(), true /* rsvp */,
                          KCalendarCore::Attendee::NeedsAction, role));
    } else {
        it->setRole(role);
    }
}

// use explicit notebook uid so we don't need to assume the events involved being added there.
// the related notebook is just needed to associate updates to some plugin/account
static void updateAttendees(const KCalendarCore::Incidence::Ptr &event,
                            const QList<KCalendarCore::Person> &required,
                            const QList<KCalendarCore::Person> &optional,
                            const QString &notebookUid)
{
    if (notebookUid.isEmpty()) {
        qWarning() << "No notebook passed, refusing to send event updates from random source";
        return;
    }

    // set the notebook email address as the organizer email address
    // if no explicit organizer is set (i.e. assume we are the organizer).
    KCalendarCore::Person organizer = event->organizer();
    if (organizer.email().isEmpty()) {
        organizer.setEmail(CalendarManager::instance()->getNotebookEmail(notebookUid));
        if (!organizer.email().isEmpty())
            event->setOrganizer(organizer);
    }

    KCalendarCore::Attendee::List attendees = event->attendees();
    KCalendarCore::Attendee::List::Iterator it = attendees.begin();
    while (it != attendees.end()) {
        bool remove = true;
        remove = remove
            && (std::find_if(required.constBegin(), required.constEnd(),
                             [it] (const KCalendarCore::Person &data)
                             {return data.email() == it->email();}) == required.constEnd());
        remove = remove
            && (std::find_if(optional.constBegin(), optional.constEnd(),
                             [it] (const KCalendarCore::Person &data)
                             {return data.email() == it->email();}) == optional.constEnd());
        // if there are non-participants getting update as FYI, or chair for any reason,
        // avoid sending them the cancel
        remove = remove && it->role() != KCalendarCore::Attendee::ReqParticipant
            && it->role() != KCalendarCore::Attendee::OptParticipant;
        if (remove)
            it = attendees.erase(it);
        else
            ++it;
    }
    for (const KCalendarCore::Person &contact : required)
        updateAttendee(attendees, contact, KCalendarCore::Attendee::ReqParticipant);
    for (const KCalendarCore::Person &contact : optional)
        updateAttendee(attendees, contact, KCalendarCore::Attendee::OptParticipant);
    event->setAttendees(attendees);
}

void CalendarEventModification::updateIncidence() const
{
    CalendarEvent::updateIncidence();
    if (m_attendeesSet)
        updateAttendees(mIncidence.data, m_requiredAttendees, m_optionalAttendees, mIncidence.notebookUid);
}

void CalendarEventModification::save()
{
    updateIncidence();
    CalendarManager::instance()->saveModification(mIncidence);
}

CalendarChangeInformation * CalendarEventModification::replaceOccurrence(CalendarEventOccurrence *occurrence)
{
    updateIncidence();
    return CalendarManager::instance()->replaceOccurrence(mIncidence, occurrence);
}
