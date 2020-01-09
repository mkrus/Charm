#ifndef EVENTMODELTESTS_H
#define EVENTMODELTESTS_H

#include <QObject>

class EventModelTests : public QObject
{
    Q_OBJECT

public:
    EventModelTests();

private Q_SLOTS:
    void setAllEventsTest();
    void modifyEventsTest();
    void modelTest();
};

#endif
