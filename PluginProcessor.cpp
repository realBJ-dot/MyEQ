#include "PluginProcessor.h"

//======================== Parameter Layout ========================
juce::AudioProcessorValueTreeState::ParameterLayout MyEQAudioProcessor::makeParams()
{
    using R = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    auto freq = [] (float lo, float hi) { return R(lo, hi, 0.0f, 0.25f); }; // skewed

    // Low shelf
    p.emplace_back (std::make_unique<juce::AudioParameterFloat>("LS_FREQ", "LowShelf Freq", freq(20.f, 250.f), 120.f));
    p.emplace_back (std::make_unique<juce::AudioParameterFloat>("LS_GAIN", "LowShelf Gain", R(-15.f, 15.f), 0.f));
    p.emplace_back (std::make_unique<juce::AudioParameterFloat>("LS_Q",    "LowShelf Q",    R(0.3f, 4.0f), 0.7f));
    p.emplace_back (std::make_unique<juce::AudioParameterBool> ("LS_BYP",  "LowShelf Bypass", false));

    // Peak 1
    p.emplace_back (std::make_unique<juce::AudioParameterFloat>("P1_FREQ", "Peak1 Freq", freq(150.f, 2000.f), 750.f));
    p.emplace_back (std::make_unique<juce::AudioParameterFloat>("P1_GAIN", "Peak1 Gain", R(-15.f, 15.f), 0.f));
    p.emplace_back (std::make_unique<juce::AudioParameterFloat>("P1_Q",    "Peak1 Q",    R(0.3f, 4.0f), 1.0f));
    p.emplace_back (std::make_unique<juce::AudioParameterBool> ("P1_BYP",  "Peak1 Bypass", false));

    // Peak 2
    p.emplace_back (std::make_unique<juce::AudioParameterFloat>("P2_FREQ", "Peak2 Freq", freq(1000.f, 12000.f), 3000.f));
    p.emplace_back (std::make_unique<juce::AudioParameterFloat>("P2_GAIN", "Peak2 Gain", R(-15.f, 15.f), 0.f));
    p.emplace_back (std::make_unique<juce::AudioParameterFloat>("P2_Q",    "Peak2 Q",    R(0.3f, 4.0f), 1.0f));
    p.emplace_back (std::make_unique<juce::AudioParameterBool> ("P2_BYP",  "Peak2 Bypass", false));

    // High shelf
    p.emplace_back (std::make_unique<juce::AudioParameterFloat>("HS_FREQ", "HighShelf Freq", freq(4000.f, 20000.f), 8000.f));
    p.emplace_back (std::make_unique<juce::AudioParameterFloat>("HS_GAIN", "HighShelf Gain", R(-15.f, 15.f), 0.f));
    p.emplace_back (std::make_unique<juce::AudioParameterFloat>("HS_Q",    "HighShelf Q",    R(0.3f, 4.0f), 0.7f));
    p.emplace_back (std::make_unique<juce::AudioParameterBool> ("HS_BYP",  "HighShelf Bypass", false));

    // HP/LP
    p.emplace_back (std::make_unique<juce::AudioParameterFloat>("HP_FREQ", "High-Pass Freq", freq(20.f, 400.f), 0.f));
    p.emplace_back (std::make_unique<juce::AudioParameterBool> ("HP_BYP",  "High-Pass Bypass", true));

    p.emplace_back (std::make_unique<juce::AudioParameterFloat>("LP_FREQ", "Low-Pass Freq",  freq(4000.f, 20000.f), 20000.f));
    p.emplace_back (std::make_unique<juce::AudioParameterBool> ("LP_BYP",  "Low-Pass Bypass", true));

    return { p.begin(), p.end() };
}

//======================== Ctor / Dtor =========================
MyEQAudioProcessor::MyEQAudioProcessor()
: AudioProcessor (BusesProperties().withInput ("Input",  juce::AudioChannelSet::stereo(), true)
                                   .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
  apvts (*this, nullptr, "PARAMS", makeParams())
{}

//======================== Boilerplate =========================
bool MyEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo()
     || layouts.getMainOutputChannelSet()!= juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void MyEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = 2;

    for (auto* proc : { &lowShelf, &peak1, &peak2, &highShelf, &hp, &lp })
        proc->prepare (spec);

    updateAllCoeffs();
    prepared = true;
}

void MyEQAudioProcessor::updateAllCoeffs()
{
    auto Fs = spec.sampleRate > 0 ? spec.sampleRate : 44100.0;

    auto lsF = apvts.getRawParameterValue("LS_FREQ")->load();
    auto lsG = apvts.getRawParameterValue("LS_GAIN")->load();
    auto lsQ = apvts.getRawParameterValue("LS_Q")->load();
    *lowShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf (Fs, lsF, lsQ, juce::Decibels::decibelsToGain(lsG));

    auto p1F = apvts.getRawParameterValue("P1_FREQ")->load();
    auto p1G = apvts.getRawParameterValue("P1_GAIN")->load();
    auto p1Q = apvts.getRawParameterValue("P1_Q")->load();
    *peak1.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (Fs, p1F, p1Q, juce::Decibels::decibelsToGain(p1G));

    auto p2F = apvts.getRawParameterValue("P2_FREQ")->load();
    auto p2G = apvts.getRawParameterValue("P2_GAIN")->load();
    auto p2Q = apvts.getRawParameterValue("P2_Q")->load();
    *peak2.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (Fs, p2F, p2Q, juce::Decibels::decibelsToGain(p2G));

    auto hsF = apvts.getRawParameterValue("HS_FREQ")->load();
    auto hsG = apvts.getRawParameterValue("HS_GAIN")->load();
    auto hsQ = apvts.getRawParameterValue("HS_Q")->load();
    *highShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (Fs, hsF, hsQ, juce::Decibels::decibelsToGain(hsG));

    auto hpF = apvts.getRawParameterValue("HP_FREQ")->load();
    if (hpF <= 0.0f) hpF = 20.0f; // guard
    *hp.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass (Fs, hpF, juce::MathConstants<float>::sqrt2);

    auto lpF = apvts.getRawParameterValue("LP_FREQ")->load();
    if (lpF >= (float) (Fs * 0.49)) lpF = (float) (Fs * 0.49);
    *lp.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass (Fs, lpF, juce::MathConstants<float>::sqrt2);
}

void MyEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (!prepared) return;

    // Recalc every block (simple first pass). You can add change-detection later.
    updateAllCoeffs();

    auto ctx = juce::dsp::ProcessContextReplacing<float> (juce::dsp::AudioBlock<float> (buffer));

    const bool lsByp = apvts.getRawParameterValue("LS_BYP")->load() > 0.5f;
    const bool p1Byp = apvts.getRawParameterValue("P1_BYP")->load() > 0.5f;
    const bool p2Byp = apvts.getRawParameterValue("P2_BYP")->load() > 0.5f;
    const bool hsByp = apvts.getRawParameterValue("HS_BYP")->load() > 0.5f;
    const bool hpByp = apvts.getRawParameterValue("HP_BYP")->load() > 0.5f;
    const bool lpByp = apvts.getRawParameterValue("LP_BYP")->load() > 0.5f;

    if (!hpByp)      hp.process (ctx);
    if (!lsByp)      lowShelf.process (ctx);
    if (!p1Byp)      peak1.process (ctx);
    if (!p2Byp)      peak2.process (ctx);
    if (!hsByp)      highShelf.process (ctx);
    if (!lpByp)      lp.process (ctx);
}

void MyEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream mos (destData, true);
    apvts.state.writeToStream (mos);
}

void MyEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData (data, (size_t) sizeInBytes);
    if (tree.isValid())
        apvts.state = tree;
}

//======================== Editor =========================
juce::AudioProcessorEditor* MyEQAudioProcessor::createEditor()
{
    // Quick start: JUCE’s built-in generic editor (sliders auto-generated)
    return new juce::GenericAudioProcessorEditor (*this);
}

// Factory (JUCE macro)
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MyEQAudioProcessor();
}
