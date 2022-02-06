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

CalendarEvent::CalendarEvent(CalendarManager *manager, KCalendarCore::Incidence::Ptr incidence, const CalendarData::Notebook *notebook)
    : QObject(manager)
    , mManager(manager)
    , mRSVP(false)
    , mExternalInvitation(false)
    , mOwnerStatus(CalendarEvent::ResponseUnspecified)
{
    connect(mManager, &CalendarManager::notebookColorChanged, this, &CalendarEvent::notebookColorChanged);
    // connect(mManager, SIGNAL(eventUidChanged(QString,QString)),
    //         this, SLOT(eventUidChanged(QString,QString)));
    setIncidence(incidence, notebook);
}

CalendarEvent::~CalendarEvent()
{
}

void CalendarEvent::setIncidence(KCalendarCore::Incidence::Ptr incidence, const CalendarData::Notebook *notebook)
{
    const bool mAllDayChanged = mIncidence->allDay() != incidence->allDay();
    const bool mSummaryChanged = mIncidence->summary() != incidence->summary();
    const bool mDescriptionChanged = mIncidence->description() != incidence->description();
    const bool mDtEndChanged = mIncidence->type() == KCalendarCore::IncidenceBase::TypeEvent
        && incidence->type() == KCalendarCore::IncidenceBase::TypeEvent
        && mIncidence.staticCast<KCalendarCore::Event>()->dtEnd() != incidence.staticCast<KCalendarCore::Event>()->dtEnd();
    const bool mLocationChanged = mIncidence->location() != incidence->location();
    const bool mSecrecyChanged = mIncidence->secrecy() != incidence->secrecy();
    const bool mStatusChanged = mIncidence->status() != incidence->status();
    const bool mDtStartChanged = mIncidence->dtStart() != incidence->dtStart();

    mIncidence = incidence;
    mCalendarUid = notebook->uid;
    mReadOnly = notebook->readOnly;

    CalendarEvent::Recur oldRecur = mRecur;
    mRecur = CalendarUtils::convertRecurrence(mIncidence);

    int oldReminder = mReminder;
    mReminder = CalendarUtils::getReminder(mIncidence);

    QDateTime oldReminderDateTime = mReminderDateTime;
    mReminderDateTime = CalendarUtils::getReminderDateTime(mIncidence);

    CalendarEvent::SyncFailure oldSyncFailure = mSyncFailure;
    mSyncFailure = CalendarUtils::convertSyncFailure(mIncidence);

    CalendarEvent::Response oldOwnerStatus = mOwnerStatus;
    bool oldRSVP = mRSVP;
    mRSVP = CalendarUtils::getResponse(mIncidence, notebook->emailAddress, &mOwnerStatus);

    bool oldExternalInvitation = mExternalInvitation;
    mExternalInvitation = CalendarUtils::getExternalInvitation(mIncidence->organizer().email(), *notebook);

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
}

QString CalendarEvent::displayLabel() const
{
    return mIncidence->summary();
}

QString CalendarEvent::description() const
{
    return mIncidence->description();
}

QDateTime CalendarEvent::startTime() const
{
    // Cannot return the date time directly here. If UTC, the QDateTime
    // will be in UTC also and the UI will convert it to local when displaying
    // the time, while in every other case, it set the QDateTime in
    // local zone.
    const QDateTime dt = mIncidence->dtStart();
    return QDateTime(dt.date(), dt.time());
}

QDateTime CalendarEvent::endTime() const
{
    if (mIncidence->type() == KCalendarCore::IncidenceBase::TypeEvent) {
        const QDateTime dt = mIncidence.staticCast<KCalendarCore::Event>()->dtEnd();
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
    return toTimeSpec(mIncidence->dtStart());
}

Qt::TimeSpec CalendarEvent::endTimeSpec() const
{
    return mIncidence->type() == KCalendarCore::IncidenceBase::TypeEvent
        ? toTimeSpec(mIncidence.staticCast<KCalendarCore::Event>()->dtEnd()) : Qt::LocalTime;
}

QString CalendarEvent::startTimeZone() const
{
    return QString::fromLatin1(mIncidence->dtStart().timeZone().id());
}

QString CalendarEvent::endTimeZone() const
{
    return mIncidence->type() == KCalendarCore::IncidenceBase::TypeEvent
        ? QString::fromLatin1(mIncidence.staticCast<KCalendarCore::Event>()->dtEnd().timeZone().id()) : QString();
}

bool CalendarEvent::allDay() const
{
    return mIncidence->allDay();
}

CalendarEvent::Recur CalendarEvent::recur() const
{
    return mRecur;
}

QDateTime CalendarEvent::recurEndDate() const
{
    KCalendarCore::RecurrenceRule *defaultRule = mIncidence->recurrence()->defaultRRule();
    if (defaultRule) {
        return QDateTime(defaultRule->endDt().date());
    } else {
        return QDateTime();
    }
}

bool CalendarEvent::hasRecurEndDate() const
{
    return recurEndDate().isValid();
}

CalendarEvent::Days CalendarEvent::recurWeeklyDays() const
{
    return CalendarUtils::convertDayPositions(mIncidence);
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
    return mIncidence->uid();
}

QString CalendarEvent::color() const
{
    return mManager->getNotebookColor(mCalendarUid);
}

bool CalendarEvent::readOnly() const
{
    return mReadOnly;
}

QString CalendarEvent::calendarUid() const
{
    return mCalendarUid;
}

QString CalendarEvent::location() const
{
    return mIncidence->location();
}

CalendarEvent::Secrecy CalendarEvent::secrecy() const
{
    return CalendarUtils::convertSecrecy(mIncidence);
}

CalendarEvent::Status CalendarEvent::status() const
{
    return CalendarUtils::convertStatus(mIncidence);
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

bool CalendarEvent::sendResponse(int response)
{
    return mManager->sendResponse(CalendarData::Incidence{mIncidence, mCalendarUid}, (Response)response);
}

void CalendarEvent::deleteEvent()
{
    mManager->deleteEvent(mIncidence->uid(), mIncidence->recurrenceId(), QDateTime());
    mManager->save();
}

QDateTime CalendarEvent::recurrenceId() const
{
    return mIncidence->recurrenceId();
}

QString CalendarEvent::recurrenceIdString() const
{
    if (mIncidence->hasRecurrenceId()) {
        return CalendarUtils::recurrenceIdToString(mIncidence->recurrenceId());
    } else {
        return QString();
    }
}

// Returns the event as a iCalendar string
QString CalendarEvent::iCalendar(const QString &prodId) const
{
    return mManager->convertEventToICalendarSync(mIncidence->uid(), prodId);
}

void CalendarEvent::notebookColorChanged(QString notebookUid)
{
    if (mCalendarUid == notebookUid)
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
