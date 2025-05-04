cmake_minimum_required(VERSION 3.27)

cmake_policy(SET CMP0071 NEW)
cmake_policy(SET CMP0076 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0079 NEW)

if (WIN32)
	set(CMAKE_CXX_STANDARD 20) 
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
elseif(APPLE)
	set(CMAKE_MACOSX_RPATH 1)
endif()

project("starlightapp")
enable_language(CXX)

find_package(OpenVDB CONFIG REQUIRED)
find_package(GDAL CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_subdirectory("extern/starlight") 

set(${PROJECT_NAME}_SOURCE 
	"src/main.cpp"
 	"src/Application.cpp"
	"src/Volume.cpp"
	"src/Terrain.cpp"
	"src/VolumeRenderer.cpp"
	"src/OffscreenRenderer.cpp"
	"src/ScreenMaterial.cpp"
	"src/VolumeRendererCleanup.cpp"
	"src/AABBInfo.cpp"
	"src/CameraInfo.cpp"
	"src/Screen.cpp"
	"src/VolumeSampler.cpp"
	"src/SampledVolumeTexture.cpp"
	"src/TerrainInfoFile.cpp"
	"src/TerrainChunk.cpp"
	"src/MathHelpers.cpp"
	"src/TerrainChunkInfo.cpp"
	"src/TerrainGrid.cpp"
	"src/FogControlInfo.cpp"
)

set(${PROJECT_NAME}_HEADERS 
	"include/Application.hpp"
	"include/Volume.hpp"
	"include/Terrain.hpp"
	"include/VolumeRenderer.hpp"
	"include/OffscreenRenderer.hpp"
	"include/ScreenMaterial.hpp"
	"include/VolumeRendererCleanup.hpp"
	"include/AABBInfo.hpp"
	"include/CameraInfo.hpp"
	"include/Screen.hpp"
	"include/VolumeSampler.hpp"
	"include/SampledVolumeTexture.hpp"
	"include/TerrainInfoFile.hpp"
	"include/TerrainChunk.hpp"
	"include/MathHelpers.hpp"
	"include/TerrainChunkInfo.hpp"
	"include/TerrainGrid.hpp"
	"include/FogControlInfo.hpp"
)

add_executable(${PROJECT_NAME} "${${PROJECT_NAME}_SOURCE};${${PROJECT_NAME}_HEADERS}")

target_include_directories(${PROJECT_NAME}
	PRIVATE "include"
)

if (WIN32)
	cmake_policy(SET CMP0091 NEW)

	target_link_libraries(${PROJECT_NAME}
		PRIVATE 
			starlight
			OpenVDB::openvdb
			GDAL::GDAL
			nlohmann_json::nlohmann_json
	)
else()
	find_package(Blosc CONFIG REQUIRED)

	target_link_libraries(${PROJECT_NAME}
		PRIVATE 
			starlight
			OpenVDB::openvdb
			GDAL::GDAL
			nlohmann_json::nlohmann_json
			blosc_static
	)

	set_target_properties(${PROJECT_NAME} PROPERTIES 
		CXX_STANDARD 20
		CXX_STANDARD_REQUIRED ON
	)
endif()

find_program(CPPCHECK_EXECUTABLE cppcheck)
if (CPPCHECK_EXECUTABLE)
	set(CPPCHECK_FILES "")

	foreach(FILE "${${PROJECT_NAME}_SOURCE}" "${PROJECT_NAME}_HEADERS")
		file(TO_CMAKE_PATH "${FILE}" UNIX_PATH)
		list(APPEND CPPCHECK_FILES "${UNIX_PATH}")
	endforeach()

	add_custom_target("${PROJECT_NAME}_cppcheck"
		COMMAND ${CPPCHECK_EXECUTABLE} 
			--enable=all 
			-q
			--inline-suppr
			--suppress=missingIncludeSystem
			--language=c++
			--std=c++20
			--template=gcc
			-I "include"
			${CPPCHECK_FILES}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "Running Cppcheck"
		VERBATIM
	)

	add_dependencies("${PROJECT_NAME}_cppcheck" ${CMAKE_PROJECT_NAME})
endif()

#TODO: NEED BETTER SOLUTION FOR MEDIA FILES
set(STARLIGHT_APP_BUILDER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/StarlightAppBuilder/")
set(PREP_MEDIA_CMD "${STARLIGHT_APP_BUILDER_DIR}/scripts/PrepareMedia.py")
add_custom_target(
	always_run_target ALL 
	DEPENDS ${${PROJECT_NAME}_SOURCE}
)

set(FINAL_APPLICATION_DIR "")
if (MSVC)
	set(FINAL_APPLICATION_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}")
else()
	set(FINAL_APPLICATION_DIR "${CMAKE_CURRENT_BINARY_DIR}")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_custom_command(
		TARGET
			always_run_target
		POST_BUILD
		COMMAND 
			python ${PREP_MEDIA_CMD} --builddir=${FINAL_APPLICATION_DIR} --mediadir=${CMAKE_CURRENT_SOURCE_DIR}/media --depsdir=${STARLIGHT_APP_BUILDER_DIR}/deps --fastest
	)
else()
	add_custom_command(
		TARGET
			always_run_target
		POST_BUILD
		COMMAND 
			python ${PREP_MEDIA_CMD} --builddir=${FINAL_APPLICATION_DIR} --mediadir=${CMAKE_CURRENT_SOURCE_DIR}/media --depsdir=${STARLIGHT_APP_BUILDER_DIR}/deps
	)
endif()

