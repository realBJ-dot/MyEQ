#pragma once
#include <JuceHeader.h>

class MyEQAudioProcessor : public juce::AudioProcessor
{
public:
    MyEQAudioProcessor();
    ~MyEQAudioProcessor() override = default;

    // JUCE boilerplate
    const juce::String getName() const override { return "MyEQ"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    using IIR = juce::dsp::IIR::Filter<float>;
    using Coeffs = juce::dsp::IIR::Coefficients<float>;
    template<typename T> using Dup = juce::dsp::ProcessorDuplicator<IIR, Coeffs>;

    // 4 bands + HP/LP
    Dup<IIR> lowShelf, peak1, peak2, highShelf, hp, lp;

    juce::dsp::ProcessSpec spec{};
    bool prepared { false };

    void updateAllCoeffs();

    // Parameter layout
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParams();
};
