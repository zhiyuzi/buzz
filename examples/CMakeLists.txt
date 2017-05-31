add_executable(echo-server EchoServer.cpp)
target_link_libraries(echo-server buzz pthread)

add_executable(daytime-server EchoServer.cpp)
target_link_libraries(daytime-server buzz pthread)

add_executable(timer Timer.cpp)
target_link_libraries(timer buzz pthread)