SET(NAME bibParser)




#############################################
# Sources.
#############################################

SET(BIB_PARSER_SRC src/bibParser.cc)

#############################################
# Targets.
#############################################
add_library(${NAME} ${BIB_PARSER_SRC})
target_include_directories(${NAME} PUBLIC include/)
target_link_libraries(${NAME} bibtex-spirit)



