/*
 * Copyright (C) 2015 Jolla Ltd.
 * Contact: Petri M. Gerdt <petri.gerdt@jollamobile.com>
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

#ifndef UTILS_H
#define UTILS_H

#include "calendarevent.h"
#include "calendardata.h"

// kCalCore
#include <event.h>
#include <calendar.h>

namespace CalendarUtils {

CalendarEvent::Recur convertRecurrence(const KCalCore::Event::Ptr &event);
CalendarEvent::Secrecy convertSecrecy(const KCalCore::Event::Ptr &event);
int getReminder(const KCalCore::Event::Ptr &event);
QList<CalendarData::Attendee> getEventAttendees(const KCalCore::Event::Ptr &event);
QList<QObject*> convertAttendeeList(const QList<CalendarData::Attendee> &list);
CalendarData::EventOccurrence getNextOccurrence(const KCalCore::Event::Ptr &event,
                                                const QDateTime &start = QDateTime::currentDateTime());
bool importFromFile(const QString &fileName, KCalCore::Calendar::Ptr calendar);
bool importFromIcsRawData(const QByteArray &icsData, KCalCore::Calendar::Ptr calendar);
CalendarEvent::Response convertPartStat(KCalCore::Attendee::PartStat status);
KCalCore::Attendee::PartStat convertResponse(CalendarEvent::Response response);

} // namespace CalendarUtils

#endif // UTILS_H
