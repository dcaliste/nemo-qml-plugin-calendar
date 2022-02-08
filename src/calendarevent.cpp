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

#include <QDateTime>
#include <QTimeZone>
#include <QBitArray>

#include "calendareventoccurrence.h"
#include "calendarmanager.h"
#include "calendarutils.h"

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

CalendarEvent::CalendarEvent(const CalendarEvent &event, QObject *parent)
    : QObject(parent)
    , mIncidence(CalendarData::Incidence{KCalendarCore::Incidence::Ptr(event.mIncidence.data->clone()), event.mIncidence.notebookUid})
{
    cacheIncidence();
}

CalendarEvent::~CalendarEvent()
{
}

int CalendarEvent::getIncidenceReminder() const
{
    KCalendarCore::Alarm::List alarms = mIncidence.data->alarms();

    int seconds = -1; // Any negative values means "no reminder"
    for (int ii = 0; ii < alarms.count(); ++ii) {
        if (alarms.at(ii)->type() == KCalendarCore::Alarm::Procedure)
            continue;
        const KCalendarCore::Alarm::Ptr &alarm = alarms.at(ii);
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

QDateTime CalendarEvent::getIncidenceReminderDateTime() const
{
    for (const KCalendarCore::Alarm::Ptr &alarm : mIncidence.data->alarms()) {
        if (alarm && alarm->type() == KCalendarCore::Alarm::Display && alarm->hasTime()) {
            return alarm->time();
        }
    }

    return QDateTime();
}

CalendarEvent::Recur CalendarEvent::getIncidenceRecurrence() const
{
    if (!mIncidence.data->recurs())
        return CalendarEvent::RecurOnce;

    if (mIncidence.data->recurrence()->rRules().count() != 1)
        return CalendarEvent::RecurCustom;

    ushort rt = mIncidence.data->recurrence()->recurrenceType();
    int freq = mIncidence.data->recurrence()->frequency();

    if (rt == KCalendarCore::Recurrence::rDaily && freq == 1) {
        return CalendarEvent::RecurDaily;
    } else if (rt == KCalendarCore::Recurrence::rWeekly && freq == 1) {
        if (mIncidence.data->recurrence()->days().count(true) == 0) {
            return CalendarEvent::RecurWeekly;
        } else {
            return CalendarEvent::RecurWeeklyByDays;
        }
    } else if (rt == KCalendarCore::Recurrence::rWeekly && freq == 2 && mIncidence.data->recurrence()->days().count(true) == 0) {
        return CalendarEvent::RecurBiweekly;
    } else if (rt == KCalendarCore::Recurrence::rMonthlyDay && freq == 1) {
        return CalendarEvent::RecurMonthly;
    } else if (rt == KCalendarCore::Recurrence::rMonthlyPos && freq == 1) {
        const QList<KCalendarCore::RecurrenceRule::WDayPos> monthPositions = mIncidence.data->recurrence()->monthPositions();
        if (monthPositions.length() == 1
            && monthPositions.first().day() == mIncidence.data->dtStart().date().dayOfWeek()) {
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

CalendarEvent::Days CalendarEvent::getIncidenceDayPositions() const
{
    if (!mIncidence.data->recurs())
        return CalendarEvent::NoDays;

    if (mIncidence.data->recurrence()->rRules().count() != 1)
        return CalendarEvent::NoDays;

    if (mIncidence.data->recurrence()->recurrenceType() != KCalendarCore::Recurrence::rWeekly
        || mIncidence.data->recurrence()->frequency() != 1)
        return CalendarEvent::NoDays;

    const CalendarEvent::Day week[7] = {CalendarEvent::Monday,
                                        CalendarEvent::Tuesday,
                                        CalendarEvent::Wednesday,
                                        CalendarEvent::Thursday,
                                        CalendarEvent::Friday,
                                        CalendarEvent::Saturday,
                                        CalendarEvent::Sunday};

    const QList<KCalendarCore::RecurrenceRule::WDayPos> monthPositions = mIncidence.data->recurrence()->monthPositions();
    CalendarEvent::Days days = CalendarEvent::NoDays;
    for (QList<KCalendarCore::RecurrenceRule::WDayPos>::ConstIterator it = monthPositions.constBegin();
         it != monthPositions.constEnd(); ++it) {
        days |= week[it->day() - 1];
    }
    return days;
}

void CalendarEvent::cacheIncidence()
{
    mRecur = getIncidenceRecurrence();
    mRecurWeeklyDays = getIncidenceDayPositions();
    mRecurEndDate = QDate();
    if (mIncidence.data->recurs()) {
        KCalendarCore::RecurrenceRule *defaultRule = mIncidence.data->recurrence()->defaultRRule();
        if (defaultRule) {
            mRecurEndDate = defaultRule->endDt().date();
        }
    }
    mReminder = getIncidenceReminder();
    mReminderDateTime = getIncidenceReminderDateTime();
}

void CalendarEvent::updateIncidenceRecurrence() const
{
    CalendarEvent::Recur oldRecur = getIncidenceRecurrence();

    if (oldRecur != mRecur
        || mRecur == CalendarEvent::RecurMonthlyByDayOfWeek
        || mRecur == CalendarEvent::RecurMonthlyByLastDayOfWeek
        || mRecur == CalendarEvent::RecurWeeklyByDays) {
        switch (mRecur) {
        case CalendarEvent::RecurOnce:
            mIncidence.data->recurrence()->clear();
            break;
        case CalendarEvent::RecurDaily:
            mIncidence.data->recurrence()->setDaily(1);
            break;
        case CalendarEvent::RecurWeekly:
            mIncidence.data->recurrence()->setWeekly(1);
            break;
        case CalendarEvent::RecurBiweekly:
            mIncidence.data->recurrence()->setWeekly(2);
            break;
        case CalendarEvent::RecurWeeklyByDays: {
            QBitArray rDays(7);
            rDays.setBit(0, mRecurWeeklyDays & CalendarEvent::Monday);
            rDays.setBit(1, mRecurWeeklyDays & CalendarEvent::Tuesday);
            rDays.setBit(2, mRecurWeeklyDays & CalendarEvent::Wednesday);
            rDays.setBit(3, mRecurWeeklyDays & CalendarEvent::Thursday);
            rDays.setBit(4, mRecurWeeklyDays & CalendarEvent::Friday);
            rDays.setBit(5, mRecurWeeklyDays & CalendarEvent::Saturday);
            rDays.setBit(6, mRecurWeeklyDays & CalendarEvent::Sunday);
            mIncidence.data->recurrence()->setWeekly(1, rDays);
            break;
        }
        case CalendarEvent::RecurMonthly:
            mIncidence.data->recurrence()->setMonthly(1);
            break;
        case CalendarEvent::RecurMonthlyByDayOfWeek: {
            mIncidence.data->recurrence()->setMonthly(1);
            const QDate at(mIncidence.data->dtStart().date());
            mIncidence.data->recurrence()->addMonthlyPos((at.day() - 1) / 7 + 1, at.dayOfWeek());
            break;
        }
        case CalendarEvent::RecurMonthlyByLastDayOfWeek: {
            mIncidence.data->recurrence()->setMonthly(1);
            const QDate at(mIncidence.data->dtStart().date());
            mIncidence.data->recurrence()->addMonthlyPos(-1, at.dayOfWeek());
            break;
        }
        case CalendarEvent::RecurYearly:
            mIncidence.data->recurrence()->setYearly(1);
            break;
        case CalendarEvent::RecurCustom:
            // Unable to handle the recurrence rules, keep the existing ones.
            break;
        }
    }
    if (mRecur != CalendarEvent::RecurOnce) {
        mIncidence.data->recurrence()->setEndDate(mRecurEndDate);
        if (!mRecurEndDate.isValid()) {
            // Recurrence/RecurrenceRule don't have separate method to clear the end date, and currently
            // setting invalid date doesn't make the duration() indicate recurring infinitely.
            mIncidence.data->recurrence()->setDuration(-1);
        }
    }
}

void CalendarEvent::updateIncidenceReminder() const
{
    if (getIncidenceReminder() == mReminder && getIncidenceReminderDateTime() == mReminderDateTime)
        return;

    const KCalendarCore::Alarm::List &alarms = mIncidence.data->alarms();
    for (int ii = 0; ii < alarms.count(); ++ii) {
        if (alarms.at(ii)->type() == KCalendarCore::Alarm::Procedure)
            continue;
        mIncidence.data->removeAlarm(alarms.at(ii));
    }

    // negative reminder seconds means "no reminder", so only
    // deal with positive (or zero = at time of event) reminders.
    if (mReminder >= 0) {
        KCalendarCore::Alarm::Ptr alarm = mIncidence.data->newAlarm();
        alarm->setEnabled(true);
        // backend stores as "offset to dtStart", i.e negative if reminder before event.
        alarm->setStartOffset(-1 * mReminder);
        alarm->setType(KCalendarCore::Alarm::Display);
    } else if (mReminderDateTime.isValid()) {
        KCalendarCore::Alarm::Ptr alarm = mIncidence.data->newAlarm();
        alarm->setEnabled(true);
        alarm->setTime(mReminderDateTime);
        alarm->setType(KCalendarCore::Alarm::Display);
    }
}

void CalendarEvent::updateIncidence() const
{
    updateIncidenceReminder();
    updateIncidenceRecurrence();
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
    KCalendarCore::Incidence::Secrecy s = mIncidence.data->secrecy();
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

CalendarEvent::Status CalendarEvent::status() const
{
    switch (mIncidence.data->status()) {
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

CalendarEvent::SyncFailure CalendarEvent::syncFailure() const
{
    const QString &syncFailure = mIncidence.data->customProperty("VOLATILE", "SYNC-FAILURE");
    if (syncFailure.compare("upload", Qt::CaseInsensitive) == 0) {
        return CalendarEvent::UploadFailure;
    } else if (syncFailure.compare("update", Qt::CaseInsensitive) == 0) {
        return CalendarEvent::UpdateFailure;
    } else if (syncFailure.compare("delete", Qt::CaseInsensitive) == 0) {
        return CalendarEvent::DeleteFailure;
    }
    return CalendarEvent::NoSyncFailure;
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

QList<QObject*> CalendarEvent::attendees()
{
    // List of Person object are computed on demand-only.
    if (mAttendees.isEmpty())
        mAttendees = getIncidenceAttendees();
    return mAttendees;
}

QList<QObject*> CalendarEvent::getIncidenceAttendees() const
{
    QList<QObject*> result;

    const KCalendarCore::Person &organizer = mIncidence.data->organizer();
    if (!organizer.email().isEmpty())
        result.append(new Person(organizer.name(), organizer.email(), true,
                                 Person::ChairParticipant, Person::UnknownParticipation));

    const KCalendarCore::Attendee::List attendees = mIncidence.data->attendees();
    for (const KCalendarCore::Attendee &attendee : attendees) {
        if (attendee.name() == organizer.name()
            && attendee.email() == organizer.email()) {
            // avoid duplicate info
            continue;
        }
        Person::AttendeeRole role;
        switch (attendee.role()) {
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
        switch (attendee.status()) {
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
        result.append(new Person(attendee.name(), attendee.email(), false,
                                 role, status));
    }

    return result;
}

CalendarStoredEvent::CalendarStoredEvent(CalendarManager *manager, const CalendarData::Incidence &incidence)
    : CalendarEvent(manager)
    , mManager(manager)
{
    connect(mManager, &CalendarManager::notebookColorChanged,
            this, &CalendarStoredEvent::notebookColorChanged);
    // connect(mManager, SIGNAL(eventUidChanged(QString,QString)),
    //         this, SLOT(eventUidChanged(QString,QString)));
    if (incidence.data && !incidence.data->uid().isEmpty())
        setIncidence(incidence);
}

CalendarStoredEvent::~CalendarStoredEvent()
{
}

bool CalendarStoredEvent::isValid() const
{
    return mIncidence.data && !mIncidence.data->uid().isEmpty();
}

static CalendarEvent::Response convertPartStat(KCalendarCore::Attendee::PartStat status)
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

static CalendarEvent::Response convertResponseType(const QString &responseType)
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

bool CalendarStoredEvent::getIncidenceResponse(const QString &calendarEmail, CalendarEvent::Response *response) const
{
    // It would be good to set the attendance status directly in the event within the plugin,
    // however in some cases the account email and owner attendee email won't necessarily match
    // (e.g. in the case where server-side aliases are defined but unknown to the plugin).
    // So we handle this here to avoid "missing" some status changes due to owner email mismatch.
    // This defaults to QString() -> ResponseUnspecified in case the property is undefined
    if (response)
        *response = convertResponseType(mIncidence.data->nonKDECustomProperty("X-EAS-RESPONSE-TYPE"));
    const KCalendarCore::Attendee::List attendees = mIncidence.data->attendees();
    for (const KCalendarCore::Attendee &calAttendee : attendees) {
        if (calAttendee.email() == calendarEmail) {
            if (response && convertPartStat(calAttendee.status()) != CalendarEvent::ResponseUnspecified) {
                // Override the ResponseType
                *response = convertPartStat(calAttendee.status());
            }
            //TODO: KCalendarCore::Attendee::RSVP() returns false even if response was requested for some accounts like Google.
            // We can use attendee role until the problem is not fixed (probably in Google plugin).
            // To be updated later when google account support for responses is added.
            return calAttendee.RSVP();// || calAttendee->role() != KCalendarCore::Attendee::Chair;
        }
    }
    return false;
}

static bool getExternalInvitation(const QString &organizerEmail, const CalendarData::Notebook &notebook)
{
    return !organizerEmail.isEmpty() && organizerEmail != notebook.emailAddress
            && !notebook.sharedWith.contains(organizerEmail);
}

void CalendarStoredEvent::setIncidence(const CalendarData::Incidence &incidence)
{
    CalendarEvent::Recur oldRecur = mRecur;
    CalendarEvent::Days oldDays = mRecurWeeklyDays;
    int oldReminder = mReminder;
    QDateTime oldReminderDateTime = mReminderDateTime;
    CalendarEvent::SyncFailure oldSyncFailure = syncFailure();

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
    mRSVP = getIncidenceResponse(notebook.emailAddress, &mOwnerStatus);

    bool oldExternalInvitation = mExternalInvitation;
    mExternalInvitation = getExternalInvitation(mIncidence.data->organizer().email(), notebook);

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
    if (syncFailure() != oldSyncFailure)
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

bool CalendarStoredEvent::sendResponse(int response)
{
    return mManager->sendResponse(mIncidence.data->uid(), mIncidence.data->recurrenceId(), (Response)response);
}

void CalendarStoredEvent::deleteEvent()
{
    mManager->deleteEvent(mIncidence.data->uid(), mIncidence.data->recurrenceId(), QDateTime());
    mManager->save();
}

// Returns the event as a iCalendar string
QString CalendarStoredEvent::iCalendar(const QString &prodId) const
{
    return mManager->convertEventToICalendarSync(mIncidence.data->uid(), prodId);
}

void CalendarStoredEvent::notebookColorChanged(QString notebookUid)
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
