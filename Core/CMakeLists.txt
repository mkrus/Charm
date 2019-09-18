SET(
    CharmCore_SRCS
    CharmConstants.cpp
    CharmExceptions.cpp
    Controller.cpp
    Dates.cpp
    SqlRaiiTransactor.cpp
    Configuration.cpp
    SqlStorage.cpp
    Event.cpp
    Task.cpp
    TaskModel.cpp
    State.cpp
    CharmDataModel.cpp
    TimeSpans.cpp
    CharmCommand.cpp
    XmlSerialization.cpp
)
ecm_qt_declare_logging_category(CharmCore_SRCS HEADER charm_core_debug.h IDENTIFIER CHARM_CORE_LOG CATEGORY_NAME org.kdab.charm.core)
ADD_LIBRARY( CharmCore STATIC ${CharmCore_SRCS} )

kde_target_enable_exceptions( CharmCore PUBLIC )

TARGET_LINK_LIBRARIES( CharmCore Qt5::Core Qt5::Widgets Qt5::Sql Qt5::Xml)
