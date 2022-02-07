/*
 * Copyright (c) 2013 - 2019 Jolla Ltd.
 * Copyright (c) 2020 - 2021 Open Mobile Platform LLC.
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

#include "calendarevent.h"

#include <QQmlInfo>
#include <QDateTime>
#include <QTimeZone>

#include "calendarutils.h"
#include "calendarmanager.h"

CalendarEvent::CalendarEvent(QObject *parent)
    : QObject(parent)
    , mIncidence(CalendarData::Incidence{KCalendarCore::Incidence::Ptr(new KCalendarCore::Event), QString()})
{
}

CalendarEvent::CalendarEvent(KCalendarCore::Incidence::Ptr incidence, QObject *parent)
    : QObject(parent)
    , mIncidence(CalendarData::Incidence{incidence ? KCalendarCore::Incidence::Ptr(incidence) : KCalendarCore::Incidence::Ptr(new KCalendarCore::Event), QString()})
{
    cacheIncidence();
}

CalendarEvent::CalendarEvent(const CalendarData::Incidence &incidence, QObject *parent)
    : QObject(parent)
    , mIncidence(CalendarData::Incidence{KCalendarCore::Incidence::Ptr(incidence.data->clone()), incidence.notebookUid})
{
    cacheIncidence();
}

CalendarEvent::~CalendarEvent()
{
}

void CalendarEvent::cacheIncidence()
{
    mRecur = CalendarUtils::convertRecurrence(mIncidence.data);
    mRecurWeeklyDays = CalendarUtils::convertDayPositions(mIncidence.data);
    mRecurEndDate = QDate();
    if (mIncidence.data->recurs()) {
        KCalendarCore::RecurrenceRule *defaultRule = mIncidence.data->recurrence()->defaultRRule();
        if (defaultRule) {
            mRecurEndDate = defaultRule->endDt().date();
        }
    }
    mReminder = CalendarUtils::getReminder(mIncidence.data);
    mReminderDateTime = CalendarUtils::getReminderDateTime(mIncidence.data);
    mSyncFailure = CalendarUtils::convertSyncFailure(mIncidence.data);
}

void CalendarEvent::updateIncidence()
{
    updateReminder(mIncidence.data, mReminder, mReminderDateTime);
    updateRecurrence(mIncidence.data, mRecur, mRecurWeeklyDays, mRecurEndDate);
}

QString CalendarEvent::displayLabel() const
{
    return mIncidence.data->summary();
}

QString CalendarEvent::description() const
{
    return mIncidence.data->description();
}

QDateTime CalendarEvent::startTime() const
{
    // Cannot return the date time directly here. If UTC, the QDateTime
    // will be in UTC also and the UI will convert it to local when displaying
    // the time, while in every other case, it set the QDateTime in
    // local zone.
    const QDateTime dt = mIncidence.data->dtStart();
    return QDateTime(dt.date(), dt.time());
}

QDateTime CalendarEvent::endTime() const
{
    if (mIncidence.data->type() == KCalendarCore::IncidenceBase::TypeEvent) {
        const QDateTime dt = mIncidence.data.staticCast<KCalendarCore::Event>()->dtEnd();
        return QDateTime(dt.date(), dt.time());
    }
    return QDateTime();
}

static Qt::TimeSpec toTimeSpec(const QDateTime &dt)
{
    if (dt.timeZone() == QTimeZone::utc()) {
        return Qt::UTC;
    }

    return dt.timeSpec();
}

Qt::TimeSpec CalendarEvent::startTimeSpec() const
{
    return toTimeSpec(mIncidence.data->dtStart());
}

Qt::TimeSpec CalendarEvent::endTimeSpec() const
{
    return mIncidence.data->type() == KCalendarCore::IncidenceBase::TypeEvent
        ? toTimeSpec(mIncidence.data.staticCast<KCalendarCore::Event>()->dtEnd()) : Qt::LocalTime;
}

QString CalendarEvent::startTimeZone() const
{
    return QString::fromLatin1(mIncidence.data->dtStart().timeZone().id());
}

QString CalendarEvent::endTimeZone() const
{
    return mIncidence.data->type() == KCalendarCore::IncidenceBase::TypeEvent
        ? QString::fromLatin1(mIncidence.data.staticCast<KCalendarCore::Event>()->dtEnd().timeZone().id()) : QString();
}

bool CalendarEvent::allDay() const
{
    return mIncidence.data->allDay();
}

CalendarEvent::Recur CalendarEvent::recur() const
{
    return mRecur;
}

QDateTime CalendarEvent::recurEndDate() const
{
    return QDateTime(mRecurEndDate);
}

bool CalendarEvent::hasRecurEndDate() const
{
    return mRecurEndDate.isValid();
}

CalendarEvent::Days CalendarEvent::recurWeeklyDays() const
{
    return mRecurWeeklyDays;
}

int CalendarEvent::reminder() const
{
    return mReminder;
}

QDateTime CalendarEvent::reminderDateTime() const
{
    return mReminderDateTime;
}

QString CalendarEvent::uniqueId() const
{
    return mIncidence.data->uid();
}

QString CalendarEvent::color() const
{
    return mNotebookColor;
}

bool CalendarEvent::readOnly() const
{
    return mReadOnly;
}

QString CalendarEvent::calendarUid() const
{
    return mIncidence.notebookUid;
}

QString CalendarEvent::location() const
{
    return mIncidence.data->location();
}

CalendarEvent::Secrecy CalendarEvent::secrecy() const
{
    return CalendarUtils::convertSecrecy(mIncidence.data);
}

CalendarEvent::Status CalendarEvent::status() const
{
    return CalendarUtils::convertStatus(mIncidence.data);
}

CalendarEvent::SyncFailure CalendarEvent::syncFailure() const
{
    return mSyncFailure;
}

CalendarEvent::Response CalendarEvent::ownerStatus() const
{
    return mOwnerStatus;
}

bool CalendarEvent::rsvp() const
{
    return mRSVP;
}

bool CalendarEvent::externalInvitation() const
{
    return mExternalInvitation;
}

QDateTime CalendarEvent::recurrenceId() const
{
    return mIncidence.data->recurrenceId();
}

QString CalendarEvent::recurrenceIdString() const
{
    if (mIncidence.data->hasRecurrenceId()) {
        return CalendarUtils::recurrenceIdToString(mIncidence.data->recurrenceId());
    } else {
        return QString();
    }
}

CalendarManagedEvent::CalendarManagedEvent(CalendarManager *manager, const CalendarData::Incidence &incidence)
    : CalendarEvent(manager)
    , mManager(manager)
{
    connect(mManager, &CalendarManager::notebookColorChanged,
            this, &CalendarManagedEvent::notebookColorChanged);
    // connect(mManager, SIGNAL(eventUidChanged(QString,QString)),
    //         this, SLOT(eventUidChanged(QString,QString)));
    if (!incidence.data->uid().isEmpty())
        setIncidence(incidence);
}

CalendarManagedEvent::~CalendarManagedEvent()
{
}

void CalendarManagedEvent::setIncidence(const CalendarData::Incidence &incidence)
{
    CalendarEvent::Recur oldRecur = mRecur;
    CalendarEvent::Days oldDays = mRecurWeeklyDays;
    int oldReminder = mReminder;
    QDateTime oldReminderDateTime = mReminderDateTime;
    CalendarEvent::SyncFailure oldSyncFailure = mSyncFailure;

    const bool mAllDayChanged = mIncidence.data->allDay() != incidence.data->allDay();
    const bool mSummaryChanged = mIncidence.data->summary() != incidence.data->summary();
    const bool mDescriptionChanged = mIncidence.data->description() != incidence.data->description();
    const bool mDtEndChanged = mIncidence.data->type() == KCalendarCore::IncidenceBase::TypeEvent
        && incidence.data->type() == KCalendarCore::IncidenceBase::TypeEvent
        && mIncidence.data.staticCast<KCalendarCore::Event>()->dtEnd() != incidence.data.staticCast<KCalendarCore::Event>()->dtEnd();
    const bool mLocationChanged = mIncidence.data->location() != incidence.data->location();
    const bool mSecrecyChanged = mIncidence.data->secrecy() != incidence.data->secrecy();
    const bool mStatusChanged = mIncidence.data->status() != incidence.data->status();
    const bool mDtStartChanged = mIncidence.data->dtStart() != incidence.data->dtStart();
    const CalendarData::Notebook &notebook = mManager->notebook(incidence.notebookUid);
    const bool mNbColorChanged = notebook.color != mNotebookColor;

    mReadOnly = notebook.readOnly;
    mNotebookColor = notebook.color;

    CalendarEvent::Response oldOwnerStatus = mOwnerStatus;
    bool oldRSVP = mRSVP;
    mRSVP = CalendarUtils::getResponse(mIncidence.data, notebook.emailAddress, &mOwnerStatus);

    bool oldExternalInvitation = mExternalInvitation;
    mExternalInvitation = CalendarUtils::getExternalInvitation(mIncidence.data->organizer().email(), notebook);

    mIncidence = incidence;
    cacheIncidence();

    if (mAllDayChanged)
        emit allDayChanged();
    if (mSummaryChanged)
        emit displayLabelChanged();
    if (mDescriptionChanged)
        emit descriptionChanged();
    if (mDtStartChanged)
        emit startTimeChanged();
    if (mDtEndChanged)
        emit endTimeChanged();
    if (mLocationChanged)
        emit locationChanged();
    if (mSecrecyChanged)
        emit secrecyChanged();
    if (mStatusChanged)
        emit statusChanged();
    if (mRecur != oldRecur)
        emit recurChanged();
    if (mRecurWeeklyDays != oldDays)
        emit recurWeeklyDaysChanged();
    if (mReminder != oldReminder)
        emit reminderChanged();
    if (mReminderDateTime != oldReminderDateTime)
        emit reminderDateTimeChanged();
    if (mSyncFailure != oldSyncFailure)
        emit syncFailureChanged();
    if (mRSVP != oldRSVP)
        emit rsvpChanged();
    if (mOwnerStatus != oldOwnerStatus)
        emit ownerStatusChanged();
    if (mExternalInvitation != oldExternalInvitation)
        emit externalInvitationChanged();
    if (mNbColorChanged)
        emit colorChanged();
}

bool CalendarManagedEvent::sendResponse(int response)
{
    return mManager->sendResponse(mIncidence, (Response)response);
}

void CalendarManagedEvent::deleteEvent()
{
    mManager->deleteEvent(mIncidence.data->uid(), mIncidence.data->recurrenceId(), QDateTime());
    mManager->save();
}

// Returns the event as a iCalendar string
QString CalendarManagedEvent::iCalendar(const QString &prodId) const
{
    return mManager->convertEventToICalendarSync(mIncidence.data->uid(), prodId);
}

void CalendarManagedEvent::notebookColorChanged(QString notebookUid)
{
    if (mIncidence.notebookUid == notebookUid)
        emit colorChanged();
}

// void CalendarEvent::eventUidChanged(QString oldUid, QString newUid)
// {
//     if (mUniqueId == oldUid) {
//         mUniqueId = newUid;
//         emit uniqueIdChanged();
//         // Event uid changes when the event is moved between notebooks, calendar uid has changed
//         emit calendarUidChanged();
//     }
// }
