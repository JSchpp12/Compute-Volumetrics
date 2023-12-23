cmake_minimum_required(VERSION 3.8)

cmake_policy(SET CMP0071 NEW)
cmake_policy(SET CMP0076 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0079 NEW)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MACOSX_RPATH 1)

project("starlightapp")
add_subdirectory("extern/starlight")

include(ExternalProject)
ExternalProject_Add(oneTBB 
	GIT_REPOSITORY https://github.com/oneapi-src/oneTBB.git
	GIT_TAG v2021.4.1
	CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
	PREFIX ${CMAKE_CURRENT_BINARY_DIR}/extern/oneTBB 
)

ExternalProject_Add(openVDB
	GIT_REPOSITORY https://github.com/AcademySoftwareFoundation/openvdb.git
	GIT_TAG v11.0.0
	DEPENDS oneTBB
	CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DOPENVDB_CORE_SHARED=OFF -DOPENVDB_CORE_STATIC=ON
	PREFIX ${CMAKE_CURRENT_BINARY_DIR}/extern/openVDB
)
ExternalProject_Get_Property(openVDB install_dir)
set(install_dir openVDB_install_dir)

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
		${openVDB_install_dir}/include
)

target_link_libraries(${PROJECT_NAME}
	starlight
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

