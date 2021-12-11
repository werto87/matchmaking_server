target_sources(${TARGET_NAME} PRIVATE 
${CMAKE_CURRENT_LIST_DIR}/util.hxx
util.cxx
)

add_subdirectory(database)
add_subdirectory(game)
add_subdirectory(server)
add_subdirectory(logic)
add_subdirectory(pw_hash)