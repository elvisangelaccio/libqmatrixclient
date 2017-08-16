cmake_minimum_required(VERSION 3.0)

project(qmatrixclient)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 REQUIRED Network Gui)

# Set up source files
set(libqmatrixclient_SRCS
   connectiondata.cpp
   connection.cpp
   logging.cpp
   room.cpp
   user.cpp
   settings.cpp
   events/event.cpp
   events/roommessageevent.cpp
   events/roomnameevent.cpp
   events/roomaliasesevent.cpp
   events/roomcanonicalaliasevent.cpp
   events/roommemberevent.cpp
   events/roomtopicevent.cpp
   events/typingevent.cpp
   events/receiptevent.cpp
   jobs/basejob.cpp
   jobs/checkauthmethods.cpp
   jobs/passwordlogin.cpp
   jobs/sendeventjob.cpp
   jobs/postreceiptjob.cpp
   jobs/joinroomjob.cpp
   jobs/leaveroomjob.cpp
   jobs/roommessagesjob.cpp
   jobs/syncjob.cpp
   jobs/mediathumbnailjob.cpp
   jobs/logoutjob.cpp
    )

set(example_SRCS examples/qmc-example.cpp)

add_library(qmatrixclient STATIC ${libqmatrixclient_SRCS})
target_link_libraries(qmatrixclient Qt5::Core Qt5::Network Qt5::Gui)

add_executable(qmc-example ${example_SRCS})
target_link_libraries(qmc-example Qt5::Core qmatrixclient)
