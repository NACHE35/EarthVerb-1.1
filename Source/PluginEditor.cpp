#include "PluginEditor.h"

EarthVerbEditor::EarthVerbEditor (EarthVerbProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    addAndMakeVisible (globe);
    for (auto* s : { &timeS, &delayS, &sizeS, &decayS }) setupSmall (*s);

    aMix   = std::make_unique<Attach> (proc.apvts, "mix",   globe);
    aTime  = std::make_unique<Attach> (proc.apvts, "time",  timeS);
    aDelay = std::make_unique<Attach> (proc.apvts, "delay", delayS);
    aSize  = std::make_unique<Attach> (proc.apvts, "size",  sizeS);
    aDecay = std::make_unique<Attach> (proc.apvts, "decay", decayS);

    globe.setDoubleClickReturnValue (true, 0.35);
    timeS.setDoubleClickReturnValue  (true, 2.5);
    delayS.setDoubleClickReturnValue (true, 0.0);
    sizeS.setDoubleClickReturnValue  (true, 0.5);
    decayS.setDoubleClickReturnValue (true, 0.4);

    setSize (640, 620);
}

EarthVerbEditor::~EarthVerbEditor() { setLookAndFeel (nullptr); }

void EarthVerbEditor::setupSmall (juce::Slider& s)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    s.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffbcd7ff));
    s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (s);
}

void EarthVerbEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff02040c), 0, 0,
                                             juce::Colour (0xff0b1430), 0, bounds.getHeight(), false));
    g.fillAll();

    juce::Random rnd (42);
    for (int i = 0; i < 120; ++i)
    {
        g.setColour (juce::Colours::white.withAlpha (0.15f + 0.5f * rnd.nextFloat()));
        const float sz = 0.8f + 1.4f * rnd.nextFloat();
        g.fillEllipse (rnd.nextFloat() * bounds.getWidth(), rnd.nextFloat() * bounds.getHeight(), sz, sz);
    }

    g.setColour (juce::Colour (0xffdff1ff));
    g.setFont (juce::Font (26.0f, juce::Font::bold));
    g.drawText ("E A R T H   V E R B", 0, 16, getWidth(), 34, juce::Justification::centred);

    g.setFont (juce::Font (13.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xff8fb8e8));
    const std::pair<juce::Slider*, const char*> labels[] = { { &timeS, "TIME" }, { &delayS, "DELAY" },
                                                              { &sizeS, "SIZE" }, { &decayS, "DECAY" } };
    for (auto& l : labels)
        g.drawText (l.second, l.first->getX(), l.first->getY() - 22, l.first->getWidth(), 20, juce::Justification::centred);

    g.drawText ("D R Y   /   W E T", 0, globe.getBottom() + 4, getWidth(), 20, juce::Justification::centred);
}

void EarthVerbEditor::resized()
{
    const int W = getWidth(), H = getHeight();
    globe.setBounds (W / 2 - 180, H / 2 - 170, 360, 360);
    const int kw = 120, kh = 150, m = 24;
    timeS.setBounds  (m,          96,          kw, kh);   // haut gauche
    delayS.setBounds (W - m - kw, 96,          kw, kh);   // haut droite
    sizeS.setBounds  (m,          H - 170,     kw, kh);   // bas gauche
    decayS.setBounds (W - m - kw, H - 170,     kw, kh);   // bas droite
}
