cmake_minimum_required(VERSION 3.26)

project(compute_volumetrics
	DESCRIPTION "Rendering application to compute fog"
	LANGUAGES C CXX
	VERSION 1.3.0
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

# Apply warning flags only to first-party targets. PRIVATE flags do not leak
# into consumers or vendored subprojects. Note: no -Werror is added here.
function(star_enable_warnings target)
	if(NOT STAR_ENABLE_WARNINGS OR NOT TARGET ${target})
		return()
	endif()

	if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
		target_compile_options(${target} PRIVATE
			-Wall
			-Wextra
			-Wpedantic
			-Wconversion
		)
	elseif(MSVC)
		target_compile_options(${target} PRIVATE /W4)
	endif()
endfunction()

option(HEADLESS "Build without presentation support" OFF)

cmake_policy(SET CMP0071 NEW)
cmake_policy(SET CMP0076 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0079 NEW)
# Normalize runtime-dependency paths before regex matching (CMake 4.3+).
if(POLICY CMP0207)
	cmake_policy(SET CMP0207 NEW)
endif()
set(CMAKE_CXX_STANDARD 23)
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

# Starlight-Terrain installs its runtime data (PROJ) into this component so it
# ships with the app's Runtime component; its dev artifacts stay in
# "Unspecified" and are excluded from the distributable.
set(STARLIGHT_TERRAIN_RUNTIME_COMPONENT Runtime)

add_subdirectory("extern/Starlight-Common")
add_subdirectory("extern/starlight")
add_subdirectory("extern/Starlight-Terrain")

if (NOT HEADLESS)
	add_subdirectory("extern/Starlight-Windowing")
endif()

star_enable_warnings(starlight)
star_enable_warnings(starlight_terrain)
star_enable_warnings(starlight_windowing)

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
	"src/TransmittanceCellPlanner.cpp"
 	"src/renderer/volume/ContainerRenderResourceData.cpp"
	"src/renderer/volume/ComputePipelineLayout.cpp"
	"src/renderer/PipelineLayoutAssembler.cpp"
	"src/renderer/volume/VolumePipelineRecipe.cpp"
	"src/renderer/VolumeRenderPhase.cpp"
	"src/VolumeRenderPhaseProvider.cpp"
	"src/render_system/fog/sync/SyncProvider.cpp"
	"src/render_system/fog/sync/wait/GatherFromCO.cpp"
	"src/render_system/fog/commands/Color.cpp"
	"src/render_system/fog/commands/PreMemoryBarrierContributor.cpp"
	"src/render_system/fog/commands/PostMemoryBarrierContributor.cpp"
	"src/render_system/fog/commands/Pass.cpp"
	"src/renderer/distance/CreatePipelines.cpp"
	"src/render_system/fog/commands/Distance.cpp"
	"src/render_system/fog/commands/ComputeContributor.cpp"
	"src/VisibilityDistanceCompute.cpp"
	"src/OffscreenRenderPhase.cpp"
	"src/OffscreenRenderPhaseProvider.cpp"
	"src/ScreenMaterial.cpp"
	"src/CameraInfo.cpp"
	"src/Screen.cpp"
 	"src/VolumeSampler.cpp"
 	"src/FogControlInfo.cpp"
	"src/FogVolumeInfo.cpp"
	"src/CopyDepthTextureToBuffer.cpp"
	"src/util/Math.cpp"
	"src/util/CmdLine.cpp"
	"src/util/VolumePath.cpp"
	"src/VolumeDataBase.cpp"
	"src/FogData.cpp"
	"src/LevelSetData.cpp"
	"src/RandomValueTexture.cpp"
	"src/RandomValueTextureController.cpp"
	"src/InteractiveMode.cpp"
	"src/HeadlessMode.cpp"
	"src/InteractiveApplication.cpp"
	"src/GUIController.cpp"
	"src/VolumeDirectoryProcessor.cpp"
	"src/VolumeFile.cpp"
	"src/VDBFileInfo.cpp"
	"src/PrepareDistanceBufferInfo.cpp"
	"src/service/ImageMetricManager.cpp"
	"src/command/image_metrics/TriggerCapture.cpp"
	"src/command/image_metrics/GetTransferCopyPass.cpp"
	"src/command/sim_controller/TriggerUpdate.cpp"
	"src/command/sim_controller/CheckIfDone.cpp"
	"src/policy/VolumetricsDeviceRequirementsProvider.cpp"
	"src/policy/EngineExitOnFlag.cpp"
	"src/service/detail/image_metric_manager/CopyDeviceToHostMemory.cpp"
	"src/service/detail/image_metric_manager/CopyCmds.cpp"
	"src/service/detail/image_metric_manager/FileWriteFunction.cpp"
	"src/service/detail/image_metric_manager/HostVisibleStorage.cpp"
	"src/service/detail/image_metric_manager/SharedBufferHandle.cpp"
	"src/service/detail/image_metric_manager/ImageMetrics.cpp"
	"src/service/detail/image_metric_manager/SharedBufferWriteDistanceMaskPayload.cpp"
	"src/service/detail/image_metric_manager/SharedBufferWriteValidityMaskPayload.cpp"
	"src/render_system/fog/ShadowDispatchResourceProvider.cpp"
	"src/render_system/fog/FogDispatcher.cpp"
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
	"src/render_system/fog/policies/ShadowDepthAcquirePolicy.cpp" 
	"src/render_system/fog/policies/ShadowDepthReleaseBackPolicy.cpp"
	"src/render_system/fog/policies/ShadowResourceResolutionPolicy.cpp" 
	"src/render_system/fog/commands/color/FrameInputPreMemoryBarrierRecorder.cpp"
	"src/render_system/fog/commands/color/PreMemoryBarrierRecorder.cpp"
 	"src/render_system/fog/commands/distance/PreDifferentFamilies.cpp" 
	"src/render_system/fog/commands/distance/PreMemoryBarrierRecorder.cpp" 
	"src/render_system/fog/commands/transmittance/TransmittancePrecompute.cpp"
	"src/render_system/fog/commands/transmittance/PreMemoryBarrierRecorder.cpp" 
	"src/service/detail/image_metric_manager/VolumeInfo_json.cpp" 
	"src/service/detail/image_metric_manager/RayDistanceStats_json.cpp"
	"src/service/detail/image_metric_manager/RayVisibilityMetrics_json.cpp"
	"src/service/detail/image_metric_manager/VisibilityDistanceInfo_json.cpp"
	"src/service/detail/image_metric_manager/ImageFilesInfo_json.cpp"
	"src/service/detail/image_metric_manager/DistanceMaskImages_json.cpp"
	"src/FogInfo_json.cpp" 
 	"src/config/InteractiveConfigInfo_json.cpp"
 	"src/config/AppConfigInfo_json.cpp"
	"src/config/AppConfigValidator.cpp"
	"src/config/AppConfigLoader.cpp"
	"src/loader/SceneLoaders.cpp"  
	"src/loader/SceneDescription.cpp" 
	"src/util/Color.cpp" 
	"src/service/detail/image_metric_manager/ImageMetrics_json.cpp"
	"src/renderer/finalization/HeadlessPhase.cpp"
	"src/renderer/finalization/HeadlessPhaseProvider.cpp"
)

set(${PROJECT_NAME}_HEADERS 
	"include/DataRoles.hpp"
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
	"include/TransmittanceCellPlanner.hpp"
	"include/render_system/fog/BarrierFactory.hpp"
	"include/render_system/fog/ShadowDispatchResourceProvider.hpp"
	"include/render_system/fog/TransmittanceMapConfig.hpp"
 	"include/renderer/volume/ContainerRenderResourceData.hpp"
	"include/renderer/volume/ComputePipelineLayout.hpp"
	"include/renderer/PipelineLayoutAssembler.hpp"
	"include/renderer/volume/VolumePipelineRecipe.hpp"
	"include/renderer/volume/VolumeFrameRoles.hpp"
	"include/renderer/VolumeRenderPhase.hpp"
	"include/VolumeRenderPhaseProvider.hpp"
	"include/renderer/distance/CreatePipelines.hpp"
	"include/VisibilityDistanceCompute.hpp"
	"include/ScreenMaterial.hpp"
	"include/CameraInfo.hpp"
	"include/Screen.hpp"
	"include/VolumeSampler.hpp"
	"include/FogControlInfo.hpp"
	"include/FogVolumeInfo.hpp"
	"include/CopyDepthTextureToBuffer.hpp"
	"include/util/Math.hpp"
	"include/util/CmdLine.hpp"
	"include/util/VolumePath.hpp"
	"include/VolumeDataBase.hpp"
	"include/FogData.hpp"
	"include/LevelSetData.hpp"
	"include/RandomValueTexture.hpp"
	"include/RandomValueTextureController.hpp"
	"include/InteractiveMode.hpp"
	"include/HeadlessMode.hpp"
	"include/InteractiveApplication.hpp"
	"include/GUIController.hpp"
 	"include/config/InteractiveConfigInfo.hpp"
	"include/config/InteractiveConfigInfo_json.hpp"
 	"include/config/AppConfigInfo.hpp"
	"include/config/AppConfigInfo_json.hpp"
	"include/config/AppConfigValidator.hpp"
	"include/config/AppConfigLoader.hpp"
	"include/VolumeDirectoryProcessor.hpp"
	"include/VolumeFile.hpp"
	"include/VDBFileInfo.hpp"
	"include/PrepareDistanceBufferInfo.hpp"
	"include/service/ImageMetricManager.hpp"
	"include/policy/VolumetricsDeviceRequirementsProvider.hpp"
	"include/policy/EngineExitOnFlag.hpp"
	"include/command/image_metrics/TriggerCapture.hpp"
	"include/command/image_metrics/GetTransferCopyPass.hpp"
	"include/command/sim_controller/TriggerUpdate.hpp"
	"include/command/sim_controller/CheckIfDone.hpp"
	"include/service/detail/image_metric_manager/CopyDeviceToHostMemory.hpp"
	"include/service/detail/image_metric_manager/CopyCmds.hpp"
	"include/service/detail/image_metric_manager/FileWriteFunction.hpp"
	"include/service/detail/image_metric_manager/HostVisibleStorage.hpp"
	"include/service/detail/image_metric_manager/SharedBufferHandle.hpp"
	"include/service/detail/image_metric_manager/SharedBufferWriteDistanceMaskPayload.hpp"
	"include/service/detail/image_metric_manager/SharedBufferWriteValidityMaskPayload.hpp"
	"include/service/detail/image_metric_manager/ImageMetrics.hpp"
	"include/service/detail/image_metric_manager/RayDistanceStats.hpp"
	"include/service/detail/image_metric_manager/RayVisibilityMetrics.hpp"
	"include/service/detail/image_metric_manager/VisibilityDistanceInfo.hpp"
	"include/service/detail/image_metric_manager/RayDistanceStats_json.hpp"
	"include/service/detail/image_metric_manager/RayVisibilityMetrics_json.hpp"
	"include/service/detail/image_metric_manager/VisibilityDistanceInfo_json.hpp"
	"include/service/detail/image_metric_manager/ImageFilesInfo.hpp"
	"include/service/detail/image_metric_manager/ImageFilesInfo_json.hpp"
	"include/service/detail/image_metric_manager/DistanceMaskImages.hpp"
	"include/service/detail/image_metric_manager/DistanceMaskImages_json.hpp"
	"include/renderer/finalization/HeadlessPhase.hpp"
	"include/renderer/finalization/HeadlessPhaseProvider.hpp"
)

add_library(${PROJECT_NAME}_lib STATIC
    ${${PROJECT_NAME}_SOURCES}
    ${${PROJECT_NAME}_HEADERS}
)
target_include_directories(${PROJECT_NAME}_lib 
	PUBLIC include
)
star_enable_warnings(${PROJECT_NAME}_lib)
if(COMMAND star_add_pch)
    star_add_pch(${PROJECT_NAME}_lib)
endif()
if (WIN32)
	cmake_policy(SET CMP0091 NEW)

	if (HEADLESS)
		target_link_libraries(${PROJECT_NAME}_lib
		PUBLIC 
			Starlight::starlight
			Starlight::terrain
			OpenVDB::openvdb
			GDAL::GDAL
			zstd::libzstd
	)
	else()
		target_link_libraries(${PROJECT_NAME}_lib
		PUBLIC
			Starlight::windowing
			Starlight::terrain
			OpenVDB::openvdb
			GDAL::GDAL
			zstd::libzstd
		)
	endif()

else()
	# add_link_options("-latomic")
	if (HEADLESS)
		target_link_libraries(${PROJECT_NAME}_lib
			PUBLIC 
				Starlight::starlight
				Starlight::terrain
				OpenVDB::openvdb
				GDAL::GDAL
				zstd::libzstd
				atomic
		)
	else()
		target_link_libraries(${PROJECT_NAME}_lib
			PUBLIC 
				starlight_windowing
				Starlight::terrain
				OpenVDB::openvdb
				GDAL::GDAL
				zstd::libzstd
				atomic
		)
	endif()
endif()

if (MSVC)
	target_compile_options(${PROJECT_NAME}_lib PUBLIC 
		$<$<CXX_COMPILER_ID:MSVC>:/bigobj /MP /EHsc /Zc:__cplusplus> 
	)
endif()

# Directory that holds the built executable together with the generated media
# and configuration.  Multi-config generators (Visual Studio) place each
# configuration in its own subdirectory, while single-config generators
# (Ninja, Makefiles) build directly in the binary directory.
if (CMAKE_CONFIGURATION_TYPES)
	set(FINAL_APPLICATION_DIR "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>")
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

# Keep the executable in the same directory as the generated media/config so
# the build tree mirrors the packaged layout and CPack can find everything.
set_target_properties(${PROJECT_NAME} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${FINAL_APPLICATION_DIR}"
)

target_include_directories(${PROJECT_NAME}
	PRIVATE "include"
)
target_link_libraries(${PROJECT_NAME}
	PRIVATE ${PROJECT_NAME}_lib
)
star_enable_warnings(${PROJECT_NAME})

set(MEDIA_STAMP "${CMAKE_BINARY_DIR}/media.stamp")

find_package(Git REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(STARLIGHT_APP_BUILDER_DIR "${CMAKE_SOURCE_DIR}/out/extern/StarlightAppBuilder" CACHE PATH "Shared StarlightAppBuilder install directory")
set(STARLIGHT_APP_BUILDER_BIN "${STARLIGHT_APP_BUILDER_DIR}/bin")
set(STARLIGHT_APP_BUILDER_SRC "${STARLIGHT_APP_BUILDER_DIR}/src")
find_program(STAR_APP_BUILDER_COMMAND
	NAMES star_app_builder
	PATHS "${STARLIGHT_APP_BUILDER_BIN}"
)

if (STAR_APP_BUILDER_COMMAND)
	message(STATUS "Found star_app_builder: ${STAR_APP_BUILDER_COMMAND}")
else()
	message(STATUS "star_app_builder not found; fetching and installing into ${STARLIGHT_APP_BUILDER_DIR}")
	file(REMOVE_RECURSE
		"${STARLIGHT_APP_BUILDER_SRC}"
		"${STARLIGHT_APP_BUILDER_BIN}"
	)
	execute_process(
		COMMAND ${GIT_EXECUTABLE} clone --depth 1
				"https://github.com/JSchpp12/StarlightAppBuilder.git"
				"${STARLIGHT_APP_BUILDER_SRC}"
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
		COMMAND_ERROR_IS_FATAL ANY
	)
	execute_process(
		COMMAND ${Python3_EXECUTABLE} -m pip install
				--upgrade
				--target "${STARLIGHT_APP_BUILDER_DIR}"
				"${STARLIGHT_APP_BUILDER_SRC}"
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
		COMMAND_ERROR_IS_FATAL ANY
	)
endif()

add_custom_target(starlight_app_builder)

set(STAR_APP_BUILDER_ENV_ARGS "PYTHONPATH=${STARLIGHT_APP_BUILDER_DIR}")
set(STAR_APP_BUILDER_EXEC_ARGS ${Python3_EXECUTABLE} -m star_app_builder)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_custom_target(prepare_media ALL
		COMMAND ${CMAKE_COMMAND} -E env
			${STAR_APP_BUILDER_ENV_ARGS}
			${STAR_APP_BUILDER_EXEC_ARGS} prep-media
			--outDir=${FINAL_APPLICATION_DIR}/media
			--inDir=${CMAKE_CURRENT_SOURCE_DIR}/media
			--fastest
		COMMENT "Running media prep"
		VERBATIM
	)
else()
	add_custom_target(prepare_media ALL
		COMMAND ${CMAKE_COMMAND} -E env
			${STAR_APP_BUILDER_ENV_ARGS}
			${STAR_APP_BUILDER_EXEC_ARGS} prep-media
			--outDir=${FINAL_APPLICATION_DIR}/media
			--inDir=${CMAKE_CURRENT_SOURCE_DIR}/media
		COMMENT "Running media prep"
		VERBATIM
	)
endif()
add_dependencies(prepare_media starlight_app_builder)

add_custom_target(create_config ALL
	COMMAND ${CMAKE_COMMAND} -E env
			${STAR_APP_BUILDER_ENV_ARGS}
			${STAR_APP_BUILDER_EXEC_ARGS} create-config
			--outDir=${FINAL_APPLICATION_DIR}
	COMMENT "Running config prep"
	VERBATIM
)
add_dependencies(create_config starlight_app_builder)

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

# System libraries that must never be copied into the package.  vcpkg has
# already deployed the app's DLLs/SOs next to the executable at build time;
# these filters only stop CMake's dependency resolver from also pulling in
# the operating system's own runtime (and, on Windows, from choking on API-set
# forwarders such as api-ms-win-*, which have no file on disk).
set(_STAR_RUNTIME_PRE_EXCLUDE)
set(_STAR_RUNTIME_POST_EXCLUDE)
if (WIN32)
    # DLL names are matched lowercased, so plain lowercase patterns suffice.
    list(APPEND _STAR_RUNTIME_PRE_EXCLUDE "^api-ms-.*" "^ext-ms-.*")
    # CMP0207 normalizes separators to '/'; the directory keeps its case.
    list(APPEND _STAR_RUNTIME_POST_EXCLUDE ".*[Ww]indows/system32/.*")
else()
    set(_STAR_RUNTIME_PRE_EXCLUDE
        ".*libc\\.so\\..*" ".*libm\\.so\\..*" ".*libpthread\\.so\\..*"
        ".*librt\\.so\\..*" ".*libdl\\.so\\..*" ".*libresolv\\.so\\..*"
        ".*ld-linux-.*\\.so\\..*" ".*libnss_.*\\.so\\..*"
        ".*libgcc_s\\.so\\..*" ".*libvulkan\\.so.*")
    set(_STAR_RUNTIME_POST_EXCLUDE ${_STAR_RUNTIME_PRE_EXCLUDE})
endif()

install(TARGETS ${PROJECT_NAME}
    RUNTIME_DEPENDENCIES
        PRE_EXCLUDE_REGEXES ${_STAR_RUNTIME_PRE_EXCLUDE}
        POST_EXCLUDE_REGEXES ${_STAR_RUNTIME_POST_EXCLUDE}
    RUNTIME DESTINATION . COMPONENT Runtime
	LIBRARY DESTINATION lib COMPONENT Runtime
	ARCHIVE DESTINATION lib COMPONENT Runtime
)
install(
    DIRECTORY "${FINAL_APPLICATION_DIR}/media"
    DESTINATION .
    COMPONENT Runtime
)
install(
    FILES "${FINAL_APPLICATION_DIR}/StarEngine.cfg"
    DESTINATION .
    COMPONENT Runtime
)

# ---------------------------------------------------------------------------
# Packaging (CPack)
# ---------------------------------------------------------------------------
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VENDOR "Compute Volumetrics")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME} ${PROJECT_VERSION}")
if (WIN32)
	set(_STAR_PACKAGE_SYSTEM "win64")
else()
	set(_STAR_PACKAGE_SYSTEM "${CMAKE_SYSTEM_NAME}")
endif()
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${_STAR_PACKAGE_SYSTEM}")

# Only the runtime component is shipped.  This keeps the vendored projects'
# static libraries, headers and CMake export files (which live in the
# "Unspecified" component) out of the distributable.
set(CPACK_COMPONENTS_ALL Runtime)
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_ARCHIVE_RUNTIME_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}")

# Portable ZIP only.
set(CPACK_GENERATOR "ZIP")

include(CPack)

include(CTest)
enable_testing()
find_package(GTest CONFIG REQUIRED)

# Optional: make a toggle to build tests
option(BUILD_UNIT_TESTS "Build unit tests" ON)

if (BUILD_UNIT_TESTS)
	add_subdirectory(tests)
	star_enable_warnings(unit_tests)
endif()
