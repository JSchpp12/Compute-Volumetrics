cmake_minimum_required(VERSION 3.26)

project(compute_volumetrics
	DESCRIPTION "Rendering application to compute fog"
	LANGUAGES C CXX
)

option(STAR_ENABLE_UBSAN "Enable UBSan" OFF)
option(STAR_ENABLE_WARNINGS "Enable Warnings" OFF)
if (STAR_ENABLE_UBSAN)
	message("STAR - Enable UBSAN")
	if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
		add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
		add_link_options(-fsanitize=undefined -lubsan)
	endif()
endif()
if (STAR_ENABLE_WARNINGS)
	message("STAR - Enable WARNINGS")

	if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
		add_compile_options(
			-Wall
			-Wextra
			-Wpedantic
			-Wconversion
			-g
		)
	endif()

	if (MSVC)
		add_compile_options(/W4 /Zi)
	endif()
endif()

option(HEADLESS "Build without presentation support" OFF)

cmake_policy(SET CMP0071 NEW)
cmake_policy(SET CMP0076 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0079 NEW)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (APPLE)
	set(CMAKE_MACOSX_RPATH 1)
endif()

find_package(Threads REQUIRED)
find_package(OpenVDB CONFIG REQUIRED)
find_package(GDAL CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)
set(starlight_common_DIR "${CMAKE_BINARY_DIR}/extern/Starlight-Common" CACHE PATH "")
add_subdirectory("extern/Starlight-Common")
add_subdirectory("extern/starlight")

if (NOT HEADLESS)
	add_subdirectory("extern/Starlight-Windowing")
endif()

set(${PROJECT_NAME}_SOURCES
	"src/main.cpp"
	"src/controller/detail/SimulationBoundsFile.cpp"
	"src/controller/detail/simulation_bounds_file/SimulationSteps.cpp"
	"src/controller/detail/simulation_bounds_file/SimulationBounds.cpp"
	"src/controller/detail/simulation_bounds_file/Reader.cpp"
	"src/controller/detail/simulation_bounds_file/Writer.cpp"
	"src/controller/CircleCameraController.cpp"
	"src/controller/SimulationController.cpp"
	"src/FogType.cpp"
	"src/VDBTransfer.cpp"
	"src/AABBTransfer.cpp"
 	"src/Application.cpp"
	"src/Volume.cpp"
	"src/Terrain.cpp"
	"src/TerrainShapeInfo.cpp"
	"src/TerrainShapeInfoLoader.cpp"
	"src/VolumeRendererCreateDescriptorsPolicy.cpp"
	"src/VolumeRenderer.cpp"
	"src/OffscreenRenderer.cpp"
	"src/ScreenMaterial.cpp"
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
	"src/CopyDepthTextureToBuffer.cpp"
	"src/FogInfo.cpp"
	"src/util/Math.cpp"
	"src/fog_info/JsonUtils.cpp"
	"src/VolumeDataBase.cpp"
	"src/FogData.cpp"
	"src/LevelSetData.cpp"
	"src/RandomValueTexture.cpp"
	"src/RandomValueTextureController.cpp"
	"src/InteractiveMode.cpp"
	"src/HeadlessMode.cpp"
	"src/InteractiveApplication.cpp"
	"src/VolumeDirectoryProcessor.cpp"
	"src/VolumeFile.cpp"
	"src/VDBFileInfo.cpp"
	"src/PrepareDistanceBufferInfo.cpp"
	"src/service/ImageMetricManager.cpp"
	"src/policy/EngineInitPolicy.cpp"
	"src/command/image_metrics/TriggerCapture.cpp"
	"src/policy/WindowedEngineInitPolicy.cpp"
	"src/service/detail/image_metric_manager/CopyDeviceToHostMemory.cpp"
	"src/service/detail/image_metric_manager/CopyCmds.cpp"
	"src/service/detail/image_metric_manager/FileWriteFunction.cpp"
	"src/service/detail/image_metric_manager/HostVisibleStorage.cpp"
	"src/service/detail/image_metric_manager/ImageMetrics.cpp"
)

set(${PROJECT_NAME}_HEADERS 
	"include/FogType.hpp"
	"include/controller/detail/SimulationBoundsFile.hpp"
	"include/controller/detail/simulation_bounds_file/SimulationSteps.hpp"
	"include/controller/detail/simulation_bounds_file/SimulationBounds.hpp"
	"include/controller/detail/simulation_bounds_file/Reader.hpp"
	"include/controller/detail/simulation_bounds_file/Writer.hpp"
	"include/controller/CircleCameraController.hpp"
	"include/controller/SimulationController.hpp"
	"include/Application.hpp"
	"include/VDBTransfer.hpp"
	"include/AABBTransfer.hpp"
	"include/Volume.hpp"
	"include/Terrain.hpp"
	"include/TerrainShapeInfo.hpp"
	"include/TerrainShapeInfoLoader.hpp"
	"include/VolumeRendererCreateDescriptorsPolicy.hpp"
	"include/VolumeRenderer.hpp"
	"include/OffscreenRenderer.hpp"
	"include/ScreenMaterial.hpp"
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
	"include/CopyDepthTextureToBuffer.hpp"
	"include/FogInfo.hpp"
	"include/util/Math.hpp"
	"include/fog_info/JsonUtils.hpp"
	"include/VolumeDataBase.hpp"
	"include/FogData.hpp"
	"include/LevelSetData.hpp"
	"include/RandomValueTexture.hpp"
	"include/RandomValueTextureController.hpp"
	"include/InteractiveMode.hpp"
	"include/HeadlessMode.hpp"
	"include/InteractiveApplication.hpp"
	"include/VolumeDirectoryProcessor.hpp"
	"include/VolumeFile.hpp"
	"include/VDBFileInfo.hpp"
	"include/PrepareDistanceBufferInfo.hpp"
	"include/service/ImageMetricManager.hpp"
	"include/policy/WindowedEngineInitPolicy.hpp"
	"include/policy/EngineInitPolicy.hpp"
	"include/command/image_metrics/TriggerCapture.hpp"
	"include/service/detail/image_metric_manager/CopyDeviceToHostMemory.hpp"
	"include/service/detail/image_metric_manager/CopyCmds.hpp"
	"include/service/detail/image_metric_manager/FileWriteFunction.hpp"
	"include/service/detail/image_metric_manager/HostVisibleStorage.hpp"
	"include/service/detail/image_metric_manager/ImageMetrics.hpp"
)

set(${PROJECT_NAME}_INCLUDE_DIRS
	"include"
)

add_executable(${PROJECT_NAME} "${${PROJECT_NAME}_SOURCES};${${PROJECT_NAME}_HEADERS}")

target_include_directories(${PROJECT_NAME}
	PRIVATE "include"
)

if (WIN32)
	cmake_policy(SET CMP0091 NEW)

	if (HEADLESS)
		target_link_libraries(${PROJECT_NAME}
		PRIVATE 
			Starlight::starlight
			OpenVDB::openvdb
			GDAL::GDAL
			TBB::tbb
			zstd::libzstd
	)
	else()
		target_link_libraries(${PROJECT_NAME}
		PRIVATE
			Starlight::windowing
			OpenVDB::openvdb
			GDAL::GDAL
			TBB::tbb
			zstd::libzstd
		)
	endif()

else()
	# add_link_options("-latomic")
	if (HEADLESS)
		target_link_libraries(${PROJECT_NAME}
			PRIVATE 
				Starlight::starlight
				OpenVDB::openvdb
				GDAL::GDAL
				TBB::tbb
				zstd::libzstd
				atomic
		)
	else()
		target_link_libraries(${PROJECT_NAME}
			PRIVATE 
				starlight_windowing
				OpenVDB::openvdb
				GDAL::GDAL
				TBB::tbb
				zstd::libzstd
				atomic
		)
	endif()
endif()

if (NOT ${HEADLESS})
    target_compile_definitions(${PROJECT_NAME} PRIVATE STAR_ENABLE_PRESENTATION "NANOVDB_USE_OPENVDB" "GLM_FORCE_DEPTH_ZERO_TO_ONE")
	message("STAR - Enabling Presentation")
else()
	target_compile_definitions(${PROJECT_NAME} PRIVATE "NANOVDB_USE_OPENVDB" "GLM_FORCE_DEPTH_ZERO_TO_ONE")
	message("STAR - Disable Presentation")
endif()

if (NOT ${HEADLESS})
    target_compile_definitions(${PROJECT_NAME} PRIVATE STAR_ENABLE_PRESENTATION)
endif()

set(STARLIGHT_APP_BUILDER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/StarlightAppBuilder/")
set(PREP_MEDIA_CMD "${STARLIGHT_APP_BUILDER_DIR}/scripts/PrepareMedia.py")
set(MEDIA_STAMP "${CMAKE_BINARY_DIR}/media.stamp")

set(FINAL_APPLICATION_DIR "")

if (MSVC)
    set(FINAL_APPLICATION_DIR "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>")
	target_compile_options(${PROJECT_NAME} PRIVATE 
		$<$<CXX_COMPILER_ID:MSVC>:/bigobj /MP /EHsc /Zc:__cplusplus> 
	)
else()
	set(FINAL_APPLICATION_DIR "${CMAKE_CURRENT_BINARY_DIR}")
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_custom_target(prepare_media ALL
		COMMAND ${Python3_EXECUTABLE} ${PREP_MEDIA_CMD} --builddir=${FINAL_APPLICATION_DIR} --mediadir=${CMAKE_CURRENT_SOURCE_DIR}/media --depsdir=${STARLIGHT_APP_BUILDER_DIR}/deps --fastest
		DEPENDS ${ALL_FILES} ${STARLIGHT_APP_BUILDER_DIR}/scripts/PrepareMedia.py
		COMMENT "Running media prep"
		VERBATIM
	)
else()
	add_custom_target(prepare_media ALL
		COMMAND ${Python3_EXECUTABLE} ${PREP_MEDIA_CMD} --builddir=${FINAL_APPLICATION_DIR} --mediadir=${CMAKE_CURRENT_SOURCE_DIR}/media --depsdir=${STARLIGHT_APP_BUILDER_DIR}/deps
		DEPENDS ${ALL_FILES} ${STARLIGHT_APP_BUILDER_DIR}/scripts/PrepareMedia.py
		COMMENT "Running media prep"
		VERBATIM
	)
endif()

# The following are for handling code cleanup/extras
find_program(CPPCHECK_EXECUTABLE cppcheck)
if (CPPCHECK_EXECUTABLE)
	set(CPPCHECK_FILES "")
    set(CPPCHECK_INCLUDES "")

	foreach(FILE "${${PROJECT_NAME}_SOURCE}" "${${PROJECT_NAME}_HEADERS}")
		file(TO_CMAKE_PATH "${FILE}" UNIX_PATH)
		list(APPEND CPPCHECK_FILES "${UNIX_PATH}")
	endforeach()

	foreach(FILE ${starlight_TARGET_INCLUDE_DIRS} ${starlight_TARGET_HEADERS})
		file(TO_CMAKE_PATH "extern/starlight/${FILE}" UNIX_PATH)
		list(APPEND CPPCHECK_FILES "${UNIX_PATH}")
	endforeach()

	foreach(DIR ${${PROJECT_NAME}_INCLUDE_DIRS})
		list(APPEND CPPCHECK_INCLUDES "-I" "${CMAKE_CURRENT_SOURCE_DIR}/${DIR}")
	endforeach()

	foreach(DIR ${starlight_TARGET_INCLUDE_DIRS})
		list(APPEND CPPCHECK_INCLUDES "-I" "${CMAKE_CURRENT_SOURCE_DIR}/extern/starlight/${DIR}")
	endforeach()

	add_custom_target("${PROJECT_NAME}_cppcheck"
		COMMAND ${CPPCHECK_EXECUTABLE} 
			--enable=all 
            --suppress=missingIncludeSystem
			-q
			--inline-suppr
			--language=c++
			--std=c++20
			--template=gcc
			${CPPCHECK_INCLUDES}
			${CPPCHECK_FILES}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "Running Cppcheck"
		VERBATIM
	)

	add_dependencies("${PROJECT_NAME}_cppcheck" ${CMAKE_PROJECT_NAME})
endif()

set(FORMAT_STAMP "${CMAKE_BINARY_DIR}/format.stamp")
set(ALL_FILES ${${PROJECT_NAME}_HEADERS} ${${PROJECT_NAME}_SOURCES})
# Convert to space-separated string
string(REPLACE ";" " " FILE_LIST "${ALL_FILES}")

add_custom_target(
    COMMAND ${Python3_EXECUTABLE} ${STARLIGHT_APP_BUILDER_DIR}/scripts/SourceCodeFormatter.py ${FILE_LIST} --dir ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/format.stamp
    DEPENDS ${ALL_FILES} ${STARLIGHT_APP_BUILDER_DIR}/scripts/SourceCodeFormatter.py
    COMMENT "Running clang-format"
)

add_custom_target(format_cpp_files
    DEPENDS ${FORMAT_STAMP}
)

include(GNUInstallDirs)
#install
install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION .
)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media
	DESTINATION .
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/StarEngine.cfg
	DESTINATION .
)


# CPack metadata
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "1.0.0")
include(CPack)