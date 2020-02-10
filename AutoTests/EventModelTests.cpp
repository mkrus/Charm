#include "EventModelTests.h"
#include "EventModel.h"
#include <random>
#include <algorithm>

#include <QTest>

Event new_event()
{
    static int counter = 1;
    Event e;
    e.setId(counter++);
    return e;
}

EventModelTests::EventModelTests()
    : QObject()
{
}

void EventModelTests::setAllEventsTest()
{
    std::random_device rd;
    std::mt19937 g(rd());

    EventModel model;

    // set all events by list with unique events
    EventList list;
    std::generate_n(std::back_inserter(list), 64, new_event);
    std::shuffle(list.begin(), list.end(), g);
    QVERIFY(model.setAllEvents(list));
    for (const auto& event : model.getEventVector())
        QVERIFY(std::find(list.begin(), list.end(), event) != list.end());

    // set all events by vector with unique events
    EventVector vector;
    std::generate_n(std::back_inserter(vector), 64, new_event);
    std::shuffle(vector.begin(), vector.end(), g);
    QVERIFY(model.setAllEvents(vector));
    for (const auto& event : model.getEventVector())
        QVERIFY(std::find(vector.begin(), vector.end(), event) != vector.end());

    // clear all events
    model.clearEvents();
    QVERIFY(model.getEventVector().size() == 0);

    // set all events with duplicates (error)
    list.push_back(list.front());
    QVERIFY(!model.setAllEvents(list));
    QVERIFY(model.getEventVector().size() == 0);
}

void EventModelTests::modifyEventsTest()
{
    EventModel model;

    EventVector vector;
    std::generate_n(std::back_inserter(vector), 64, new_event);
    model.setAllEvents(vector);

    // query existent event
    for (const auto& event : vector) {
        QVERIFY(model.eventExists(event.id()));
        auto eventForId = model.eventForId(event.id());
        QVERIFY(eventForId && eventForId->id() == event.id());
    }

    // query not existent event (error)
    QVERIFY(!model.eventExists(new_event().id()));
    QVERIFY(!model.eventForId(new_event().id()));

    // add event
    auto event = new_event();
    QVERIFY(model.addEvent(event));
    QVERIFY(model.eventExists(event.id()));

    // add existent event
    QVERIFY(!model.addEvent(model.getEventVector().front()));

    // modify event
    event = vector.front();
    event.setTaskId(event.taskId() + 1);
    QVERIFY(model.modifyEvent(event));
    auto eventForId = model.eventForId(event.id());
    QVERIFY(model.eventExists(event.id()));
    QVERIFY(eventForId && *eventForId == event);

    // modify non existent event (error)
    QVERIFY(!model.modifyEvent(new_event()));

    // delete event
    QVERIFY(model.deleteEvent(event));
    QVERIFY(!model.eventExists(event.id()));
    QVERIFY(!model.eventForId(event.id())); // query deleted event

    // delete non existent event (error)
    QVERIFY(!model.deleteEvent(new_event()));
}

void EventModelTests::modelTest()
{
    EventModel model;

    // fill the model
    EventVector vector;
    std::generate_n(std::back_inserter(vector), 64, new_event);
    model.setAllEvents(vector);

    QVERIFY(model.rowCount({}) == static_cast<int>(vector.size()));

    for (int i = 0; i < model.rowCount(model.index(0)); ++i) {
        auto index = model.index(i);
        auto id = model.data(index, EventModel::IdRole).value<EventId>();
        auto eventForIndex = model.eventForIndex(index);
        auto indexForId = model.indexForEvent(id);

        QVERIFY(eventForIndex && id == eventForIndex->id());
        QVERIFY(indexForId.isValid() && index == indexForId);

        auto byEventId = [&] (const Event& event) { return id == event.id(); };
        QVERIFY(std::find_if(vector.begin(), vector.end(), byEventId) != vector.end());
    }
}

QTEST_MAIN(EventModelTests)
