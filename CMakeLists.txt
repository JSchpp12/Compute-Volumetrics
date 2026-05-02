cmake_minimum_required(VERSION 3.26)

project(compute_volumetrics
	DESCRIPTION "Rendering application to compute fog"
	LANGUAGES C CXX
	VERSION 1.0.9
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
	"src/service/detail/simulation_controller/CameraController.cpp"
	"src/service/detail/simulation_controller/SimulationData.cpp"
	"src/service/detail/simulation_controller/SimulationSteps.cpp"
	"src/service/detail/simulation_controller/SimulationBounds.cpp"
	"src/service/detail/simulation_controller/Reader.cpp"
	"src/service/detail/simulation_controller/Writer.cpp"
	"src/service/detail/simulation_controller/FogEnabler.cpp"
	"src/service/detail/simulation_controller/FogEnabler_json.cpp"
	"src/service/SimulationController.cpp"
	"src/service/detail/simulation_controller/camera_controller/Circle.cpp"
	"src/service/detail/simulation_controller/camera_controller/Circle_json.cpp"
	"src/GetCmdBuffer.cpp"
	"src/DeclareDependentPasses.cpp"
	"src/FogType.cpp"
	"src/VDBTransfer.cpp"
	"src/AABBTransfer.cpp"
 	"src/Application.cpp"
	"src/Volume.cpp"
	"src/Terrain.cpp"
	"src/TerrainShapeInfo.cpp"
	"src/TerrainShapeInfoLoader.cpp"
	"src/renderer/volume/ContainerRenderResourceData.cpp"
	"src/renderer/volume/DescriptorBuilder.cpp"
	"src/renderer/VolumeRenderer.cpp"
	"src/render_system/fog/sync/SyncProvider.cpp"
	"src/render_system/fog/sync/wait/GatherFromCO.cpp"
	"src/render_system/fog/commands/Color.cpp"
	"src/render_system/fog/commands/PreMemoryBarrierContributor.cpp"
	"src/render_system/fog/commands/PostMemoryBarrierContributor.cpp"
	"src/render_system/fog/commands/Pass.cpp"
	"src/renderer/distance/CreatePipelines.cpp"
	"src/renderer/finalization/FinalizationRenderer.cpp"
	"src/renderer/finalization/Windowed.cpp"
	"src/renderer/finalization/Headless.cpp"
	"src/render_system/fog/commands/Distance.cpp"
	"src/render_system/fog/commands/ComputeContributor.cpp"
	"src/VisibilityDistanceCompute.cpp"
	"src/OffscreenRenderer.cpp"
	"src/ScreenMaterial.cpp"
	"src/CameraInfo.cpp"
	"src/Screen.cpp"
	"src/VolumeSampler.cpp"
	"src/TerrainInfoFile.cpp"
	"src/TerrainChunk.cpp"
	"src/MathHelpers.cpp"
	"src/TerrainChunkInfo.cpp"
	"src/TerrainGrid.cpp"
	"src/FogControlInfo.cpp"
	"src/CopyDepthTextureToBuffer.cpp"
	"src/FogInfo.cpp"
	"src/util/Math.cpp"
	"src/util/CmdLine.cpp"
	"src/util/json/MathStructs.cpp"
	"src/util/json/FogInfoStruct.cpp"
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
	"src/command/image_metrics/TriggerCapture.cpp"
	"src/command/sim_controller/TriggerUpdate.cpp"
	"src/command/sim_controller/CheckIfDone.cpp"
	"src/policy/FunctionalEngineInitPolicy.cpp"
	"src/policy/WindowedEngineInitPolicy.cpp"
	"src/policy/EngineExitOnFlag.cpp"
	"src/service/detail/image_metric_manager/CopyDeviceToHostMemory.cpp"
	"src/service/detail/image_metric_manager/CopyCmds.cpp"
	"src/service/detail/image_metric_manager/FileWriteFunction.cpp"
	"src/service/detail/image_metric_manager/HostVisibleStorage.cpp"
	"src/service/detail/image_metric_manager/ImageMetrics.cpp"
	"src/render_system/fog/FogDispatcher.cpp"
	"src/render_system/FogDispatcherBuilder.cpp"
	"src/render_system/fog/sync/signal/CalcFromFt.cpp"
	"src/render_system/fog/ChunkOrchestrator.cpp"
	"src/render_system/fog/sync/wait/WaitForPreviousChunk.cpp"
 	"src/render_system/fog/sync/wait/WaitForFirstChunk.cpp"
	"src/render_system/fog/ChunkDispatchGrid.cpp"  
	"src/render_system/fog/commands/Init.cpp" 
	"src/render_system/fog/commands/IndirectDispatch.cpp" 
	"src/render_system/fog/PipelineCacher.cpp" 
	"src/render_system/fog/commands/distance/PostDifferentFamilies.cpp" 
	"src/render_system/fog/commands/distance/PostMemoryBarrierRecorder.cpp"
	"src/render_system/fog/commands/color/PostMemoryBarrierRecorder.cpp"
	"src/render_system/fog/commands/color/PostDifferentFamilies.cpp"
	"src/render_system/fog/commands/color/PreDifferentFamilies.cpp" 
	"src/render_system/fog/commands/color/PreMemoryBarrierRecorder.cpp"
 	"src/render_system/fog/commands/distance/PreDifferentFamilies.cpp" 
	"src/render_system/fog/commands/distance/PreMemoryBarrierRecorder.cpp" 
	"src/TerrainShapeInfo_json.cpp"
)

set(${PROJECT_NAME}_HEADERS 
	"include/FogType.hpp"
	"include/service/detail/simulation_controller/CameraController.hpp"
	"include/service/detail/simulation_controller/SimulationData.hpp"
	"include/service/detail/simulation_controller/SimulationSteps.hpp"
	"include/service/detail/simulation_controller/SimulationBounds.hpp"
	"include/service/detail/simulation_controller/Reader.hpp"
	"include/service/detail/simulation_controller/Writer.hpp"
	"include/service/detail/simulation_controller/FogEnabler.hpp"
	"include/service/detail/simulation_controller/FogEnabler_json.hpp"
	"include/service/SimulationController.hpp"
	"include/service/detail/simulation_controller/camera_controller/Circle.hpp"
	"include/service/detail/simulation_controller/camera_controller/Circle_json.hpp"
	"include/GetCmdBuffer.hpp"
	"include/DeclareDependentPasses.hpp"
	"include/Application.hpp"
	"include/VDBTransfer.hpp"
	"include/AABBTransfer.hpp"
	"include/Volume.hpp"
	"include/Terrain.hpp"
	"include/TerrainShapeInfo.hpp"
	"include/TerrainShapeInfoLoader.hpp"
	"include/renderer/volume/ContainerRenderResourceData.hpp"
	"include/renderer/volume/DescriptorBuilder.hpp"
	"include/renderer/VolumeRenderer.hpp"
	"include/renderer/distance/CreatePipelines.hpp"
	"include/renderer/finalization/FinalizationRenderer.hpp"
	"include/renderer/finalization/Headless.hpp"
	"include/renderer/finalization/Windowed.hpp"
	"include/VisibilityDistanceCompute.hpp"
	"include/OffscreenRenderer.hpp"
	"include/ScreenMaterial.hpp"
	"include/CameraInfo.hpp"
	"include/Screen.hpp"
	"include/VolumeSampler.hpp"
	"include/TerrainInfoFile.hpp"
	"include/TerrainChunk.hpp"
	"include/MathHelpers.hpp"
	"include/TerrainChunkInfo.hpp"
	"include/TerrainGrid.hpp"
	"include/FogControlInfo.hpp"
	"include/CopyDepthTextureToBuffer.hpp"
	"include/FogInfo.hpp"
	"include/util/Math.hpp"
	"include/util/CmdLine.hpp"
	"include/util/json/MathStructs.hpp"
	"include/util/json/FogInfoStruct.hpp"
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
	"include/policy/FunctionalEngineInitPolicy.hpp"
	"include/policy/EngineExitOnFlag.hpp"
	"include/command/image_metrics/TriggerCapture.hpp"
	"include/command/sim_controller/TriggerUpdate.hpp"
	"include/command/sim_controller/CheckIfDone.hpp"
	"include/service/detail/image_metric_manager/CopyDeviceToHostMemory.hpp"
	"include/service/detail/image_metric_manager/CopyCmds.hpp"
	"include/service/detail/image_metric_manager/FileWriteFunction.hpp"
	"include/service/detail/image_metric_manager/HostVisibleStorage.hpp"
	"include/service/detail/image_metric_manager/ImageMetrics.hpp"
)

add_library(${PROJECT_NAME}_lib 
    ${${PROJECT_NAME}_SOURCES}
    ${${PROJECT_NAME}_HEADERS}
)
target_include_directories(${PROJECT_NAME}_lib 
	PUBLIC include
)
if (WIN32)
	cmake_policy(SET CMP0091 NEW)

	if (HEADLESS)
		target_link_libraries(${PROJECT_NAME}_lib
		PUBLIC 
			Starlight::starlight
			OpenVDB::openvdb
			GDAL::GDAL
			TBB::tbb
			zstd::libzstd
	)
	else()
		target_link_libraries(${PROJECT_NAME}_lib
		PUBLIC
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
		target_link_libraries(${PROJECT_NAME}_lib
			PUBLIC 
				Starlight::starlight
				OpenVDB::openvdb
				GDAL::GDAL
				TBB::tbb
				zstd::libzstd
				atomic
		)
	else()
		target_link_libraries(${PROJECT_NAME}_lib
			PUBLIC 
				starlight_windowing
				OpenVDB::openvdb
				GDAL::GDAL
				TBB::tbb
				zstd::libzstd
				atomic
		)
	endif()
endif()

set(FINAL_APPLICATION_DIR "")
if (MSVC)
    set(FINAL_APPLICATION_DIR "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>")
	target_compile_options(${PROJECT_NAME}_lib PUBLIC 
		$<$<CXX_COMPILER_ID:MSVC>:/bigobj /MP /EHsc /Zc:__cplusplus> 
	)
else()
	set(FINAL_APPLICATION_DIR "${CMAKE_CURRENT_BINARY_DIR}")
endif()

if (NOT ${HEADLESS})
    target_compile_definitions(${PROJECT_NAME}_lib PUBLIC STAR_ENABLE_PRESENTATION "NANOVDB_USE_OPENVDB" "GLM_FORCE_DEPTH_ZERO_TO_ONE")
	message("STAR - Enabling Presentation")
else()
	target_compile_definitions(${PROJECT_NAME}_lib PUBLIC "NANOVDB_USE_OPENVDB" "GLM_FORCE_DEPTH_ZERO_TO_ONE")
	message("STAR - Disable Presentation")
endif()

if (NOT ${HEADLESS})
    target_compile_definitions(${PROJECT_NAME}_lib PUBLIC STAR_ENABLE_PRESENTATION)
endif()

add_executable(${PROJECT_NAME} "src/main.cpp")

target_include_directories(${PROJECT_NAME}
	PRIVATE "include"
)
target_link_libraries(${PROJECT_NAME}
	PRIVATE ${PROJECT_NAME}_lib
)

set(STARLIGHT_APP_BUILDER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/StarlightAppBuilder/")
set(MEDIA_STAMP "${CMAKE_BINARY_DIR}/media.stamp")

find_package(Python3 REQUIRED COMPONENTS Interpreter)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_custom_target(prepare_media ALL
		COMMAND ${Python3_EXECUTABLE} -m star_app_builder prep-media --outDir=${FINAL_APPLICATION_DIR}/media --inDir=${CMAKE_CURRENT_SOURCE_DIR}/media --depsDir=${STARLIGHT_APP_BUILDER_DIR}/deps --fastest
		COMMENT "Running media prep"
		VERBATIM
	)
else()
	add_custom_target(prepare_media ALL
		COMMAND ${Python3_EXECUTABLE} -m star_app_builder prep-media --outDir=${FINAL_APPLICATION_DIR}/media --inDir=${CMAKE_CURRENT_SOURCE_DIR}/media --depsDir=${STARLIGHT_APP_BUILDER_DIR}/deps 
		COMMENT "Running media prep"
		VERBATIM
	)
endif()

add_custom_target(create_config ALL
	COMMAND ${Python3_EXECUTABLE} -m star_app_builder create-config --outDir=${FINAL_APPLICATION_DIR}
	COMMENT "Running config prep"
	VERBATIM
)

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

add_custom_target(format_cpp_files
    DEPENDS ${FORMAT_STAMP}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
	INSTALL_RPATH "$ORIGIN/lib"
)

include(GNUInstallDirs)

# Ensure RPATH is preserved on install (Linux)
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)          # use build rpath for build tree, install rpath for install tree
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)        # also add rpaths to where linked libs live at install time
set(CMAKE_INSTALL_RPATH "$ORIGIN/lib")             # runtime: relative to the executable's directory

install(TARGETS ${PROJECT_NAME}
    RUNTIME_DEPENDENCIES
	
       PRE_EXCLUDE_REGEXES
            # Exclude glibc core + loader
            ".*libc\\.so\\..*" ".*libm\\.so\\..*" ".*libpthread\\.so\\..*"
            ".*librt\\.so\\..*" ".*libdl\\.so\\..*" ".*libresolv\\.so\\..*"
            ".*ld-linux-.*\\.so\\..*" ".*libnss_.*\\.so\\..*"
            # (Optional) exclude libgcc_s if you’ll link it statically below
            ".*libgcc_s\\.so\\..*"
        POST_EXCLUDE_REGEXES
            # Same as above to be safe
            ".*libc\\.so\\..*" ".*libm\\.so\\..*" ".*libpthread\\.so\\..*"
            ".*librt\\.so\\..*" ".*libdl\\.so\\..*" ".*libresolv\\.so\\..*"
            ".*ld-linux-.*\\.so\\..*" ".*libnss_.*\\.so\\..*"
            ".*libgcc_s\\.so\\..*"
    RUNTIME DESTINATION .
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)
install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/media
    DESTINATION .
)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/StarEngine.cfg
    DESTINATION .
)

include(CPack)

include(CTest)
enable_testing()
find_package(GTest CONFIG REQUIRED)

# Optional: make a toggle to build tests
option(BUILD_UNIT_TESTS "Build unit tests" ON)

if (BUILD_UNIT_TESTS)
	add_subdirectory(tests)
endif()