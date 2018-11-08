cmake_minimum_required(VERSION 3.5)
project(ftsm_base_test)
set (CMAKE_CXX_STANDARD 11)

find_package(PkgConfig)
find_package (Threads)
pkg_check_modules(JSONCPP jsoncpp)
link_libraries(${JSONCPP_LIBRARIES})

include_directories(
    include
    ${JSONCPP_INCLUDE_DIRS}
)


# TESTS
add_executable(ftsm_running_test
    src/test/ftsm_running_test.cpp
    src/ftsm_base.cpp
)
target_link_libraries(ftsm_running_test ${LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable(ftsm_recovery_test
    src/test/ftsm_recovery_test.cpp
    src/ftsm_base.cpp
)
target_link_libraries(ftsm_recovery_test ${LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable(ftsm_failed_recovery_test
    src/test/ftsm_failed_recovery_test.cpp
    src/ftsm_base.cpp
)
target_link_libraries(ftsm_failed_recovery_test ${LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable(ftsm_failed_configuration_test
    src/test/ftsm_failed_configuration_test.cpp
    src/ftsm_base.cpp
)
target_link_libraries(ftsm_failed_configuration_test ${LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable(ftsm_reconfiguration_test
    src/test/ftsm_reconfiguration_test.cpp
    src/ftsm_base.cpp
)
target_link_libraries(ftsm_reconfiguration_test ${LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
