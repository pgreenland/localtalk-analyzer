cmake_minimum_required (VERSION 3.11)
project(localtalk_analyzer)

add_definitions( -DLOGIC2 )

# enable generation of compile_commands.json, helpful for IDEs to locate include files.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# custom CMake Modules are located in the cmake directory.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(ExternalAnalyzerSDK)

set(SOURCES
src/LocalTalkAnalyzer.cpp
src/LocalTalkAnalyzer.h
src/LocalTalkAnalyzerResults.cpp
src/LocalTalkAnalyzerResults.h
src/LocalTalkAnalyzerSettings.cpp
src/LocalTalkAnalyzerSettings.h
src/LocalTalkSimulationDataGenerator.cpp
src/LocalTalkSimulationDataGenerator.h
)

add_analyzer_plugin(localtalk_analyzer SOURCES ${SOURCES})
