#include "eventdata.h"

QDBusArgument &operator<<(QDBusArgument &argument, const EventData &eventData)
{
    argument.beginStructure();
    argument << eventData.calendarUid
             << eventData.instanceId
             << eventData.startTime
             << eventData.endTime
             << eventData.allDay
             << eventData.color
             << eventData.displayLabel
             << eventData.description
             << eventData.location
             << eventData.cancelled;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, EventData &eventData)
{
    argument.beginStructure();
    argument >> eventData.calendarUid
             >> eventData.instanceId
             >> eventData.startTime
             >> eventData.endTime
             >> eventData.allDay
             >> eventData.color
             >> eventData.displayLabel
             >> eventData.description
             >> eventData.location
             >> eventData.cancelled;
    argument.endStructure();
    return argument;
}
