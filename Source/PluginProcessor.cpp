#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout EarthVerbProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto attrs = [] (const String& suffix, float scale)
    {
        return AudioParameterFloatAttributes()
            .withStringFromValueFunction ([suffix, scale] (float v, int)
            {
                const float x = v * scale;
                return String (x, scale > 1.0f || x >= 100.0f ? 0 : 2) + suffix;
            })
            .withValueFromStringFunction ([scale] (const String& t) { return t.getFloatValue() / scale; });
    };

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Dry/Wet",
                 NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.35f, attrs (" %", 100.0f)));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "time", 1 }, "Time",
                 NormalisableRange<float> (0.1f, 20.0f, 0.01f, 0.4f), 2.5f, attrs (" s", 1.0f)));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "delay", 1 }, "Delay",
                 NormalisableRange<float> (0.0f, 500.0f, 0.1f, 0.5f), 0.0f, attrs (" ms", 1.0f)));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "size", 1 }, "Size",
                 NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f, attrs (" %", 100.0f)));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "decay", 1 }, "Decay",
                 NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.4f, attrs (" %", 100.0f)));
    return { p.begin(), p.end() };
}

EarthVerbProcessor::EarthVerbProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    mixP   = apvts.getRawParameterValue ("mix");
    timeP  = apvts.getRawParameterValue ("time");
    delayP = apvts.getRawParameterValue ("delay");
    sizeP  = apvts.getRawParameterValue ("size");
    decayP = apvts.getRawParameterValue ("decay");
}

void EarthVerbProcessor::prepareToPlay (double sampleRate, int)
{
    reverb.prepare (sampleRate);
    mixSmoothed.reset (sampleRate, 0.05);
    mixSmoothed.setCurrentAndTargetValue (mixP->load());
}

bool EarthVerbProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && l.getMainInputChannelSet()  == juce::AudioChannelSet::stereo();
}

void EarthVerbProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2) return;

    reverb.setParams (timeP->load(), delayP->load(), sizeP->load(), decayP->load());
    mixSmoothed.setTargetValue (mixP->load());

    auto* l = buffer.getWritePointer (0);
    auto* r = buffer.getWritePointer (1);
    const float halfPi = juce::MathConstants<float>::halfPi;

    for (int i = 0; i < n; ++i)
    {
        const float m = mixSmoothed.getNextValue();
        const float dry = std::cos (m * halfPi);
        const float wet = std::sin (m * halfPi);
        float wl, wr;
        reverb.process (l[i], r[i], wl, wr);
        l[i] = l[i] * dry + wl * wet;
        r[i] = r[i] * dry + wr * wet;
    }
}

juce::AudioProcessorEditor* EarthVerbProcessor::createEditor() { return new EarthVerbEditor (*this); }

void EarthVerbProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void EarthVerbProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new EarthVerbProcessor(); }
