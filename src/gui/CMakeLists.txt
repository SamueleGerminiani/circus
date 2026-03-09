SET(NAME gui)

#############################################
# Sources.
#############################################
SET(GUI_SRC src/gui.cc src/keywordsTable.cc src/papersTable.cc include/gui.hh)

#############################################
# Targets.
#############################################
add_library(${NAME} ${GUI_SRC})

# Turn on AUTO properties specifically for this target
set_target_properties(${NAME} PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

target_include_directories(${NAME} PUBLIC include/)
target_link_libraries(${NAME} db Qt6::Widgets Qt6::Charts)
