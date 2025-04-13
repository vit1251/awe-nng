cmake_minimum_required(VERSION 3.15)

project (awe-nng)

include_directories(${CMAKE_JS_INC})

file(GLOB SOURCE_FILES
    "src/awe-nng.c"
)

find_package (nng)
include_directories(${SQLITE3_INCLUDE_DIRS})

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})
target_link_libraries(${PROJECT_NAME} ${SQLITE3_LIBRARIES})
target_link_libraries(${PROJECT_NAME} nng)
