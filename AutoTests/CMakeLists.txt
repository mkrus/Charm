INCLUDE_DIRECTORIES( ${Charm_SOURCE_DIR} )

SET( TEST_LIBRARIES CharmCore Qt5::Test Qt5::Network)

QT5_ADD_RESOURCES( TestDataResources_SRCS TestData.qrc )

IF( APPLE )
    FIND_LIBRARY( SECURITY_FRAMEWORK FRAMEWORK required NAMES Security )
    LIST( APPEND TEST_LIBRARIES ${SECURITY_FRAMEWORK} )
ENDIF()

SET( SqlStorageTests_SRCS SqlStorageTests.cpp )
ADD_EXECUTABLE( SqlStorageTests ${SqlStorageTests_SRCS} )
TARGET_LINK_LIBRARIES( SqlStorageTests ${TEST_LIBRARIES} )
ADD_TEST( NAME SqlStorageTests COMMAND SqlStorageTests )

SET( ControllerTests_SRCS ControllerTests.cpp )
ADD_EXECUTABLE( ControllerTests ${ControllerTests_SRCS} )
TARGET_LINK_LIBRARIES( ControllerTests ${TEST_LIBRARIES} )
ADD_TEST( NAME ControllerTests COMMAND ControllerTests )

SET( EventModelFilterTests_SRCS
     ${Charm_SOURCE_DIR}/Charm/EventModelAdapter.cpp
     ${Charm_SOURCE_DIR}/Charm/EventModelFilter.cpp
     EventModelFilterTests.cpp
)
ADD_EXECUTABLE( EventModelFilterTests ${EventModelFilterTests_SRCS} )
TARGET_LINK_LIBRARIES( EventModelFilterTests ${TEST_LIBRARIES} )

SET( DatesTests_SRCS DatesTests.cpp )
ADD_EXECUTABLE( DatesTests ${DatesTests_SRCS} )
TARGET_LINK_LIBRARIES( DatesTests ${TEST_LIBRARIES} )

SET( CharmDataModelTests_SRCS CharmDataModelTests.cpp )
ADD_EXECUTABLE( CharmDataModelTests ${CharmDataModelTests_SRCS} )
TARGET_LINK_LIBRARIES( CharmDataModelTests ${TEST_LIBRARIES} )
ADD_TEST( NAME CharmDataModelTests COMMAND CharmDataModelTests )

SET(
    BackendIntegrationTests_SRCS
    BackendIntegrationTests.cpp
    TestApplication.cpp
)
ADD_EXECUTABLE( BackendIntegrationTests ${BackendIntegrationTests_SRCS} )
TARGET_LINK_LIBRARIES( BackendIntegrationTests ${TEST_LIBRARIES} )
ADD_TEST( NAME BackendIntegrationTests COMMAND BackendIntegrationTests )

SET( TaskModelTests_SRCS TaskModelTests.cpp )
ADD_EXECUTABLE( TaskModelTests ${TaskModelTests_SRCS} )
TARGET_LINK_LIBRARIES( TaskModelTests ${TEST_LIBRARIES} )
ADD_TEST( NAME TaskModelTests COMMAND TaskModelTests )

SET( TaskStructureTests_SRCS TaskStructureTests.cpp )
ADD_EXECUTABLE(
    TaskStructureTests
    ${TaskStructureTests_SRCS}
    ${TestDataResources_SRCS}
)
TARGET_LINK_LIBRARIES( TaskStructureTests ${TEST_LIBRARIES} )
ADD_TEST( NAME TaskStructureTests COMMAND TaskStructureTests )

SET( TimeSpanTests_SRCS TimeSpanTests.cpp )
ADD_EXECUTABLE( TimeSpanTests ${TimeSpanTests_SRCS} )
TARGET_LINK_LIBRARIES( TimeSpanTests ${TEST_LIBRARIES} )

SET( XmlSerializationTests_SRCS XmlSerializationTests.cpp )
ADD_EXECUTABLE(
    XmlSerializationTests
    ${XmlSerializationTests_SRCS}
    ${TestDataResources_SRCS}
)
TARGET_LINK_LIBRARIES( XmlSerializationTests ${TEST_LIBRARIES} )
ADD_TEST( NAME XmlSerializationTests COMMAND XmlSerializationTests )

SET( SqlTransactionTests_SRCS SqlTransactionTests.cpp )
ADD_EXECUTABLE( SqlTransactionTests ${SqlTransactionTests_SRCS} )
TARGET_LINK_LIBRARIES( SqlTransactionTests ${TEST_LIBRARIES} )

IF( CHARM_DATABASE_CONFIGURATION )
    ADD_TEST( NAME SqlTransactionTests COMMAND SqlTransactionTests )
    SET_PROPERTY(
        TEST SqlTransactionTests
        PROPERTY ENVIRONMENT
        "CHARM_DATABASE_CONFIGURATION=${CHARM_DATABASE_CONFIGURATION}"
    )
ENDIF()

SET( UpdateCheckerTests_SRCS ${Charm_SOURCE_DIR}/Charm/HttpClient/CheckForUpdatesJob.cpp UpdateCheckerTests.cpp )
ADD_EXECUTABLE( UpdateCheckerTests ${UpdateCheckerTests_SRCS} )
TARGET_LINK_LIBRARIES( UpdateCheckerTests ${TEST_LIBRARIES} )
ADD_TEST( NAME UpdateCheckerTests COMMAND UpdateCheckerTests )
