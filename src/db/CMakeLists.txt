SET(NAME db)



#############################################
# Sources.
#############################################

SET(DB_SRC src/db.cc)

#############################################
# Targets.
#############################################
add_library(${NAME} ${DB_SRC})
target_include_directories(${NAME} PUBLIC include/)
target_link_libraries(${NAME} bibtex-spirit SQLiteCpp sqlite3)



