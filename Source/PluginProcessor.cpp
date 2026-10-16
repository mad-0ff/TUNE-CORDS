#include "PluginProcessor.h"
#include "PluginEditor.h"

using juce::ParameterID;

juce::AudioProcessorValueTreeState::ParameterLayout TuneCordsProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    juce::StringArray keys { "Do", "Do#", juce::String::fromUTF8 ("R\xc3\xa9"), juce::String::fromUTF8 ("R\xc3\xa9#"),
                             "Mi", "Fa", "Fa#", "Sol", "Sol#", "La", "La#", "Si" };
    juce::StringArray scales;
    for (int i = 0; i < tc::numScales; ++i) scales.add (tc::scaleNames()[i]);

    p.push_back (std::make_unique<juce::AudioParameterChoice> (ParameterID { "key", 1 },   "Tonalite", keys, 0));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (ParameterID { "scale", 1 }, "Gamme", scales, 1));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (ParameterID { "voice", 1 }, "Voix",
                                                                juce::StringArray { "Auto", "Homme", "Femme" }, 0));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (ParameterID { "amount", 1 }, "Puissance",
                 juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (ParameterID { "speed", 1 }, "Vitesse",
                 juce::NormalisableRange<float> (0.0f, 400.0f, 1.0f, 0.4f), 20.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (ParameterID { "tolerance", 1 }, "Tolerance",
                 juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 15.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (ParameterID { "humanize", 1 }, "Humanize",
                 juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix",
                 juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (ParameterID { "output", 1 }, "Sortie",
                 juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));

    return { p.begin(), p.end() };
}

TuneCordsProcessor::TuneCordsProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

bool TuneCordsProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void TuneCordsProcessor::prepareToPlay (double sampleRate, int)
{
    corrector.prepare (sampleRate, juce::jmax (1, getTotalNumInputChannels()));
    setLatencySamples (corrector.getLatencySamples());
}

void TuneCordsProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, buffer.getNumSamples());

    tc::Params p;
    p.root       = (int) *apvts.getRawParameterValue ("key");
    p.scale      = (int) *apvts.getRawParameterValue ("scale");
    p.voice      = (int) *apvts.getRawParameterValue ("voice");
    p.amount     = *apvts.getRawParameterValue ("amount") * 0.01f;
    p.retuneMs   = *apvts.getRawParameterValue ("speed");
    p.toleranceC = *apvts.getRawParameterValue ("tolerance");
    p.humanize   = *apvts.getRawParameterValue ("humanize") * 0.01f;
    p.mix        = *apvts.getRawParameterValue ("mix") * 0.01f;
    p.outputGain = juce::Decibels::decibelsToGain ((float) *apvts.getRawParameterValue ("output"));
    corrector.setParams (p);

    float* chans[2] = { nullptr, nullptr };
    const int n = juce::jmin (2, buffer.getNumChannels());
    for (int c = 0; c < n; ++c) chans[c] = buffer.getWritePointer (c);
    if (n > 0) corrector.process (chans, buffer.getNumSamples());
}

juce::AudioProcessorEditor* TuneCordsProcessor::createEditor() { return new TuneCordsEditor (*this); }

void TuneCordsProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void TuneCordsProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TuneCordsProcessor(); }
