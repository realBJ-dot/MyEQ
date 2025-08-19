cmake_minimum_required(VERSION 3.22)
project(MyEQ VERSION 0.1.0)

add_subdirectory(../JUCE JUCE-build)

juce_add_plugin(MyEQ
    COMPANY_NAME "YourName"
    PLUGIN_MANUFACTURER_CODE YRNM
    PLUGIN_CODE MEQ1
    FORMATS VST3                 # AU is macOS-only
    PRODUCT_NAME "MyEQ")

juce_generate_juce_header(MyEQ)  # <-- generates JuceHeader.h

target_sources(MyEQ PRIVATE src/PluginProcessor.cpp src/PluginEditor.cpp)
target_compile_features(MyEQ PRIVATE cxx_std_20)
target_link_libraries(MyEQ PRIVATE juce::juce_audio_utils juce::juce_dsp)
