cmake_minimum_required(VERSION 3.8)

cmake_policy(SET CMP0071 NEW)
cmake_policy(SET CMP0076 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0079 NEW)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MACOSX_RPATH 1)
project("starlightapp")

#dependencies
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/libs/openVDB/lib/cmake/OpenVDB")
find_package(OpenVDB REQUIRED)
find_package(TBB REQUIRED)

add_subdirectory("extern/starlight")

set(${PROJECT_NAME}_SOURCE 
	"src/main.cpp"
 	"src/Application.cpp"
)

set(${PROJECT_NAME}_HEADERS 
)

add_executable(${PROJECT_NAME} "${${PROJECT_NAME}_SOURCE};${${PROJECT_NAME}_HEADERS}")

target_include_directories(${PROJECT_NAME}
	PRIVATE
		"include"
)

target_link_libraries(${PROJECT_NAME}
	PRIVATE starlight
	PRIVATE OpenVDB::openvdb
)

#TODO: NEED BETTER SOLUTION FOR MEDIA FILES
set(PREP_MEDIA_CMD "${CMAKE_CURRENT_SOURCE_DIR}/PrepareMedia.py")
add_custom_target(
	always_run_target ALL 
	DEPENDS ${${PROJECT_NAME}_SOURCE}
)
add_custom_command(
	TARGET
		always_run_target
	DEPENDS
		${${PROJECT_NAME}_SOURCE}
	COMMAND 
		python ${PREP_MEDIA_CMD} buildDir=${CMAKE_CURRENT_BINARY_DIR} mediaDir=${CMAKE_CURRENT_SOURCE_DIR}/media configFile=${CMAKE_CURRENT_SOURCE_DIR}/StarEngine.cfg
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
	"${CMAKE_CURRENT_LIST_DIR}/libs/openVDB/bin/openvdb.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)