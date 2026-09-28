#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "GlobeKnob.h"

class EarthVerbEditor : public juce::AudioProcessorEditor
{
public:
    explicit EarthVerbEditor (EarthVerbProcessor&);
    ~EarthVerbEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using Attach = juce::AudioProcessorValueTreeState::SliderAttachment;
    void setupSmall (juce::Slider&);

    EarthVerbProcessor& proc;
    EarthLookAndFeel laf;
    GlobeKnob globe;
    juce::Slider timeS, delayS, sizeS, decayS;
    std::unique_ptr<Attach> aMix, aTime, aDelay, aSize, aDecay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EarthVerbEditor)
};
