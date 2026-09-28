#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Reverb.h"

class EarthVerbProcessor : public juce::AudioProcessor
{
public:
    EarthVerbProcessor();
    ~EarthVerbProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Earth Verb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 20.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    EarthReverb reverb;
    juce::SmoothedValue<float> mixSmoothed;
    std::atomic<float> *mixP {}, *timeP {}, *delayP {}, *sizeP {}, *decayP {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EarthVerbProcessor)
};
