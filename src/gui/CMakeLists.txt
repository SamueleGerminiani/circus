SET(NAME gui)



#############################################
# Sources.
#############################################

SET(GUI_SRC src/gui.cc)

#############################################
# Targets.
#############################################
add_library(${NAME} ${GUI_SRC})
target_include_directories(${NAME} PUBLIC include/)
target_link_libraries(${NAME} Qt6::Widgets)



