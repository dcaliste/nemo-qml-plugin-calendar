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
#include <QBitArray>
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

CalendarEventModification::CalendarEventModification(CalendarData::Incidence data, QObject *parent)
    : QObject(parent)
    , m_event(CalendarData::Incidence{KCalendarCore::Incidence::Ptr(data.data->clone()), data.notebookUid})
    , m_attendeesSet(false)
{
    m_recur = CalendarUtils::convertRecurrence(m_event.data);
    m_recurWeeklyDays = CalendarUtils::convertDayPositions(m_event.data);
    m_reminder = CalendarUtils::getReminder(m_event.data);
    m_reminderDateTime = CalendarUtils::getReminderDateTime(m_event.data);
    if (m_event.data->recurs()) {
        KCalendarCore::RecurrenceRule *defaultRule = m_event.data->recurrence()->defaultRRule();
        if (defaultRule) {
            m_recurEndDate = defaultRule->endDt().date();
        }
    }
}

CalendarEventModification::CalendarEventModification(QObject *parent)
    : QObject(parent)
    , m_event(CalendarData::Incidence{KCalendarCore::Incidence::Ptr(new KCalendarCore::Event), QString()})
    , m_attendeesSet(false)
    , m_recur(CalendarEvent::RecurOnce)
    , m_recurWeeklyDays(CalendarEvent::NoDays)
    , m_reminder(-1)
{
}

CalendarEventModification::~CalendarEventModification()
{
}

QString CalendarEventModification::displayLabel() const
{
    return m_event.data->summary();
}

void CalendarEventModification::setDisplayLabel(const QString &displayLabel)
{
    if (m_event.data->summary() != displayLabel) {
        m_event.data->setSummary(displayLabel);
        emit displayLabelChanged();
    }
}

QString CalendarEventModification::description() const
{
    return m_event.data->description();
}

void CalendarEventModification::setDescription(const QString &description)
{
    if (m_event.data->description() != description) {
        m_event.data->setDescription(description);
        emit descriptionChanged();
    }
}

QDateTime CalendarEventModification::startTime() const
{
    return m_event.data->dtStart();
}

void CalendarEventModification::setStartTime(const QDateTime &startTime, Qt::TimeSpec spec, const QString &timezone)
{
    QDateTime newStartTimeInTz = startTime;
    updateTime(&newStartTimeInTz, spec, timezone);
    if (m_event.data->dtStart() != newStartTimeInTz
        || m_event.data->dtStart().timeSpec() != newStartTimeInTz.timeSpec()
        || (m_event.data->dtStart().timeSpec() == Qt::TimeZone
            && m_event.data->dtStart().timeZone() != newStartTimeInTz.timeZone())) {
        m_event.data->setDtStart(newStartTimeInTz);
        emit startTimeChanged();
    }
}

QDateTime CalendarEventModification::endTime() const
{
    return m_event.data->type() == KCalendarCore::IncidenceBase::TypeEvent ? m_event.data.staticCast<KCalendarCore::Event>()->dtEnd() : QDateTime();
}

void CalendarEventModification::setEndTime(const QDateTime &endTime, Qt::TimeSpec spec, const QString &timezone)
{
    QDateTime newEndTimeInTz = endTime;
    updateTime(&newEndTimeInTz, spec, timezone);
    const QDateTime &dtEnd = m_event.data->type() == KCalendarCore::IncidenceBase::TypeEvent ? m_event.data.staticCast<KCalendarCore::Event>()->dtEnd() : QDateTime();
    if (dtEnd != newEndTimeInTz
        || dtEnd.timeSpec() != newEndTimeInTz.timeSpec()
        || (dtEnd.timeSpec() == Qt::TimeZone
            && dtEnd.timeZone() != newEndTimeInTz.timeZone())) {
        if (m_event.data->type() == KCalendarCore::IncidenceBase::TypeEvent) {
            m_event.data.staticCast<KCalendarCore::Event>()->setDtEnd(newEndTimeInTz);
            emit endTimeChanged();
        }
    }
}

bool CalendarEventModification::allDay() const
{
    return m_event.data->allDay();
}

void CalendarEventModification::setAllDay(bool allDay)
{
    if (m_event.data->allDay() != allDay) {
        m_event.data->setAllDay(allDay);
        emit allDayChanged();
    }
}

CalendarEvent::Recur CalendarEventModification::recur() const
{
    return m_recur;
}

void CalendarEventModification::setRecur(CalendarEvent::Recur recur)
{
    if (m_recur != recur) {
        m_recur = recur;
        emit recurChanged();
    }
}

QDateTime CalendarEventModification::recurEndDate() const
{
    return QDateTime(m_recurEndDate);
}

bool CalendarEventModification::hasRecurEndDate() const
{
    return m_recurEndDate.isValid();
}

void CalendarEventModification::setRecurEndDate(const QDateTime &dateTime)
{
    bool wasValid = hasRecurEndDate();
    QDate date = dateTime.date();

    if (m_recurEndDate != date) {
        m_recurEndDate = date;
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

CalendarEvent::Days CalendarEventModification::recurWeeklyDays() const
{
    return m_recurWeeklyDays;
}

void CalendarEventModification::setRecurWeeklyDays(CalendarEvent::Days days)
{
    if (m_recurWeeklyDays != days) {
        m_recurWeeklyDays = days;
        emit recurWeeklyDaysChanged();
    }
}

QString CalendarEventModification::recurrenceIdString() const
{
    if (m_event.data->hasRecurrenceId()) {
        return CalendarUtils::recurrenceIdToString(m_event.data->recurrenceId());
    } else {
        return QString();
    }
}

int CalendarEventModification::reminder() const
{
    return m_reminder;
}

void CalendarEventModification::setReminder(int seconds)
{
    if (seconds != m_reminder) {
        m_reminder = seconds;
        emit reminderChanged();
    }
}

QDateTime CalendarEventModification::reminderDateTime() const
{
    return m_reminderDateTime;
}

void CalendarEventModification::setReminderDateTime(const QDateTime &dateTime)
{
    if (dateTime != m_reminderDateTime) {
        m_reminderDateTime = dateTime;
        emit reminderDateTimeChanged();
    }
}

QString CalendarEventModification::location() const
{
    return m_event.data->location();
}

void CalendarEventModification::setLocation(const QString &newLocation)
{
    if (newLocation != m_event.data->location()) {
        m_event.data->setLocation(newLocation);
        emit locationChanged();
    }
}

QString CalendarEventModification::calendarUid() const
{
    return m_event.notebookUid;
}

void CalendarEventModification::setCalendarUid(const QString &uid)
{
    if (m_event.notebookUid != uid) {
        m_event.notebookUid = uid;
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

static void setRecurrence(KCalendarCore::Incidence::Ptr &event, CalendarEvent::Recur recur, CalendarEvent::Days days)
{
    if (!event)
        return;

    CalendarEvent::Recur oldRecur = CalendarUtils::convertRecurrence(event);

    if (recur == CalendarEvent::RecurOnce)
        event->recurrence()->clear();

    if (oldRecur != recur
        || recur == CalendarEvent::RecurMonthlyByDayOfWeek
        || recur == CalendarEvent::RecurMonthlyByLastDayOfWeek
        || recur == CalendarEvent::RecurWeeklyByDays) {
        switch (recur) {
        case CalendarEvent::RecurOnce:
            break;
        case CalendarEvent::RecurDaily:
            event->recurrence()->setDaily(1);
            break;
        case CalendarEvent::RecurWeekly:
            event->recurrence()->setWeekly(1);
            break;
        case CalendarEvent::RecurBiweekly:
            event->recurrence()->setWeekly(2);
            break;
        case CalendarEvent::RecurWeeklyByDays: {
            QBitArray rDays(7);
            rDays.setBit(0, days & CalendarEvent::Monday);
            rDays.setBit(1, days & CalendarEvent::Tuesday);
            rDays.setBit(2, days & CalendarEvent::Wednesday);
            rDays.setBit(3, days & CalendarEvent::Thursday);
            rDays.setBit(4, days & CalendarEvent::Friday);
            rDays.setBit(5, days & CalendarEvent::Saturday);
            rDays.setBit(6, days & CalendarEvent::Sunday);
            event->recurrence()->setWeekly(1, rDays);
            break;
        }
        case CalendarEvent::RecurMonthly:
            event->recurrence()->setMonthly(1);
            break;
        case CalendarEvent::RecurMonthlyByDayOfWeek: {
            event->recurrence()->setMonthly(1);
            const QDate at(event->dtStart().date());
            event->recurrence()->addMonthlyPos((at.day() - 1) / 7 + 1, at.dayOfWeek());
            break;
        }
        case CalendarEvent::RecurMonthlyByLastDayOfWeek: {
            event->recurrence()->setMonthly(1);
            const QDate at(event->dtStart().date());
            event->recurrence()->addMonthlyPos(-1, at.dayOfWeek());
            break;
        }
        case CalendarEvent::RecurYearly:
            event->recurrence()->setYearly(1);
            break;
        case CalendarEvent::RecurCustom:
            // Unable to handle the recurrence rules, keep the existing ones.
            break;
        }
    }
}

static void setReminder(KCalendarCore::Incidence::Ptr &event, int seconds, const QDateTime &dateTime)
{
    if (!event)
        return;

    if (CalendarUtils::getReminder(event) == seconds
        && CalendarUtils::getReminderDateTime(event) == dateTime)
        return;

    KCalendarCore::Alarm::List alarms = event->alarms();
    for (int ii = 0; ii < alarms.count(); ++ii) {
        if (alarms.at(ii)->type() == KCalendarCore::Alarm::Procedure)
            continue;
        event->removeAlarm(alarms.at(ii));
    }

    // negative reminder seconds means "no reminder", so only
    // deal with positive (or zero = at time of event) reminders.
    if (seconds >= 0) {
        KCalendarCore::Alarm::Ptr alarm = event->newAlarm();
        alarm->setEnabled(true);
        // backend stores as "offset to dtStart", i.e negative if reminder before event.
        alarm->setStartOffset(-1 * seconds);
        alarm->setType(KCalendarCore::Alarm::Display);
    } else if (dateTime.isValid()) {
        KCalendarCore::Alarm::Ptr alarm = event->newAlarm();
        alarm->setEnabled(true);
        alarm->setTime(dateTime);
        alarm->setType(KCalendarCore::Alarm::Display);
    }
}

void CalendarEventModification::save()
{
    setReminder(m_event.data, m_reminder, m_reminderDateTime);
    setRecurrence(m_event.data, m_recur, m_recurWeeklyDays);

    if (m_recur != CalendarEvent::RecurOnce) {
        m_event.data->recurrence()->setEndDate(m_recurEndDate);
        if (!m_recurEndDate.isValid()) {
            // Recurrence/RecurrenceRule don't have separate method to clear the end date, and currently
            // setting invalid date doesn't make the duration() indicate recurring infinitely.
            m_event.data->recurrence()->setDuration(-1);
        }
    }

    CalendarManager::instance()->saveModification(m_event, m_attendeesSet,
                                                  m_requiredAttendees, m_optionalAttendees);
}

CalendarChangeInformation *
CalendarEventModification::replaceOccurrence(CalendarEventOccurrence *occurrence)
{
    return CalendarManager::instance()->replaceOccurrence(m_event, occurrence, m_attendeesSet,
                                                          m_requiredAttendees, m_optionalAttendees);
}
