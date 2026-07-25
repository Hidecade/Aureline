cmake_minimum_required(VERSION 3.22)

project(Aureline VERSION 1.0.1 LANGUAGES C CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_library(aureline_engine STATIC
    Source/DSP/Envelope.cpp
    Source/DSP/Lfo.cpp
    Source/DSP/LadderFilter.cpp
    Source/DSP/NoiseGenerator.cpp
    Source/DSP/Oscillator.cpp
    Source/Engine/AnalogEngine.cpp
    Source/Engine/FactoryPresets.cpp
    Source/Engine/PerformanceSequencer.cpp
    Source/Engine/AnalogVoice.cpp
    Source/Engine/AnalogPatch.cpp
)

target_include_directories(aureline_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Source)

add_executable(aureline_engine_tests Source/Tests/EngineTests.cpp)
target_link_libraries(aureline_engine_tests PRIVATE aureline_engine)

add_test(NAME aureline_engine_tests COMMAND aureline_engine_tests)

set(AURELINE_JUCE_DIR "" CACHE PATH "Path to a JUCE checkout")
option(AURELINE_BUILD_STANDALONE "Build the Aureline standalone application" ON)
option(AURELINE_BUILD_PLUGINS "Build the Aureline VST3 and Audio Unit plug-ins" ON)

if(AURELINE_BUILD_STANDALONE OR AURELINE_BUILD_PLUGINS)
    if(AURELINE_JUCE_DIR AND EXISTS "${AURELINE_JUCE_DIR}/CMakeLists.txt")
        set(AURELINE_RESOLVED_JUCE_DIR "${AURELINE_JUCE_DIR}")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/CMakeLists.txt")
        set(AURELINE_RESOLVED_JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../OpalineFM/external/JUCE/CMakeLists.txt")
        set(AURELINE_RESOLVED_JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../OpalineFM/external/JUCE")
    endif()

    if(AURELINE_RESOLVED_JUCE_DIR)
        add_subdirectory("${AURELINE_RESOLVED_JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")

        juce_add_binary_data(AurelineAssets
            SOURCES
                assets/aureline-wood-background.png
        )

        if(AURELINE_BUILD_PLUGINS OR AURELINE_BUILD_STANDALONE)
            set(AURELINE_PLUGIN_FORMATS)
            if(AURELINE_BUILD_PLUGINS)
                list(APPEND AURELINE_PLUGIN_FORMATS VST3)
            endif()
            if(AURELINE_BUILD_STANDALONE)
                list(APPEND AURELINE_PLUGIN_FORMATS Standalone)
            endif()
            if(APPLE AND AURELINE_BUILD_PLUGINS)
                list(APPEND AURELINE_PLUGIN_FORMATS AU)
            endif()

            juce_add_plugin(Aureline_Plugin
                PRODUCT_NAME "Aureline"
                VERSION ${PROJECT_VERSION}
                COMPANY_NAME "Hidecade"
                BUNDLE_ID "jp.hidecade.aureline.plugin"
                PLUGIN_MANUFACTURER_CODE Hica
                PLUGIN_CODE Aurl
                FORMATS ${AURELINE_PLUGIN_FORMATS}
                IS_SYNTH TRUE
                NEEDS_MIDI_INPUT TRUE
                NEEDS_MIDI_OUTPUT FALSE
                IS_MIDI_EFFECT FALSE
                COPY_PLUGIN_AFTER_BUILD FALSE
                NEEDS_WEB_BROWSER FALSE
                NEEDS_CURL FALSE
                VST3_CAN_REPLACE_VST2 FALSE
                VST3_CATEGORIES Instrument Synth
                EDITOR_WANTS_KEYBOARD_FOCUS TRUE
                ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/assets/aureline-icon.png"
                ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/assets/aureline-icon.png"
            )

            target_sources(Aureline_Plugin PRIVATE
                Source/App/MainComponent.cpp
                Source/App/MainComponent.h
                Source/Plugin/PluginProcessor.cpp
                Source/Plugin/PluginProcessor.h
            )

            target_compile_definitions(Aureline_Plugin PUBLIC
                JUCE_WEB_BROWSER=0
                JUCE_USE_CURL=0
                JUCE_ASIO=1
                JUCE_ASIO_USE_EXTERNAL_SDK=0
                JUCE_VST3_CAN_REPLACE_VST2=0
                JUCE_STANDALONE_STATE_FILE_SUFFIX=".aurelinestate"
                JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}"
            )

            target_link_libraries(Aureline_Plugin PRIVATE
                aureline_engine
                AurelineAssets
                juce::juce_audio_utils
                juce::juce_gui_extra
            PUBLIC
                juce::juce_recommended_config_flags
                juce::juce_recommended_lto_flags
                juce::juce_recommended_warning_flags
            )
        endif()
    else()
        message(STATUS "JUCE not found; Aureline standalone target disabled")
    endif()
endif()
