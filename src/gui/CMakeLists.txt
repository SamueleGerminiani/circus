SET(NAME gui)



#############################################
# Sources.
#############################################

qt6_wrap_cpp(moc_sources src/gui.cc include/gui.hh)

SET(GUI_SRC src/gui.cc)
list(APPEND GUI_SRC ${moc_sources})

#############################################
# Targets.
#############################################
add_library(${NAME} ${GUI_SRC})
target_include_directories(${NAME} PUBLIC include/)
target_link_libraries(${NAME} db Qt6::Widgets Qt6::Charts)




