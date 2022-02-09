/*
 * Copyright (C) 2013 - 2019 Jolla Ltd.
 * Copyright (C) 2020 Open Mobile Platform LLC.
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

#ifndef CALENDAREVENTQUERY_H
#define CALENDAREVENTQUERY_H

#include <QObject>
#include <QDateTime>
#include <QQmlParserStatus>

#include "calendardata.h"

class CalendarEventOccurrence;

class CalendarEventQuery : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QString uniqueId READ uniqueId WRITE setUniqueId NOTIFY uniqueIdChanged)
    Q_PROPERTY(QString recurrenceId READ recurrenceIdString WRITE setRecurrenceIdString NOTIFY recurrenceIdStringChanged)
    Q_PROPERTY(QDateTime startTime READ startTime WRITE setStartTime RESET resetStartTime NOTIFY startTimeChanged)
    Q_PROPERTY(QObject *event READ event NOTIFY eventChanged)
    Q_PROPERTY(QObject *occurrence READ occurrence NOTIFY occurrenceChanged)
    Q_PROPERTY(QList<QObject*> attendees READ attendees NOTIFY attendeesChanged)
    Q_PROPERTY(bool eventError READ eventError NOTIFY eventErrorChanged)

public:
    CalendarEventQuery();
    ~CalendarEventQuery();

    QString uniqueId() const;
    void setUniqueId(const QString &);

    QString recurrenceIdString();
    void setRecurrenceIdString(const QString &recurrenceId);
    QDateTime recurrenceId();

    QDateTime startTime() const;
    void setStartTime(const QDateTime &);
    void resetStartTime();

    QObject *event() const;
    QObject *occurrence() const;

    QList<QObject*> attendees();

    bool eventError() const;

    virtual void classBegin();
    virtual void componentComplete();

    void doRefresh(const CalendarData::Incidence &event, bool eventError);

signals:
    void uniqueIdChanged();
    void recurrenceIdStringChanged();
    void eventChanged();
    void occurrenceChanged();
    void attendeesChanged();
    void startTimeChanged();
    void eventErrorChanged();

    // Indicates that the event UID has changed in database, event has been moved between notebooks.
    // The property uniqueId will not be changed, the data pointer properties event and occurrence
    // will reset to null pointers.
    void newUniqueId(QString newUid);

private slots:
    void refresh();
    void eventUidChanged(QString oldUid, QString newUid);

private:
    bool mIsComplete;
    QString mUid;
    QDateTime mRecurrenceId;
    QDateTime mStartTime;
    bool mResetOccurrence = false;
    CalendarData::Incidence mEvent;
    CalendarEventOccurrence *mOccurrence;
    bool mEventError;
};

#endif
