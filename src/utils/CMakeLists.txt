SET(NAME all_utils)

SET(SRC src/utils.cc)
add_library(${NAME} ${SRC})

target_include_directories(${NAME} PUBLIC include/ ${Boost_INCLUDE_DIRS})

