#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

//==============================================================================
// Style "ciel étoilé analogique"
class SkyLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SkyLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getLabelFont (juce::Label&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;

    static juce::Font serif (float size, bool italic = false);
};

//==============================================================================
// Constellation des notes de la gamme + voix détectée
class NoteSky : public juce::Component
{
public:
    explicit NoteSky (tc::PitchCorrector& c, juce::AudioProcessorValueTreeState& a) : corrector (c), apvts (a) {}
    void paint (juce::Graphics&) override;
    void tick();

private:
    tc::PitchCorrector& corrector;
    juce::AudioProcessorValueTreeState& apvts;
    float trail[24] = {};
    bool trailValid[24] = {};
    int trailPos = 0;
    float shownPc = -1.0f;
};

//==============================================================================
class TuneCordsEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TuneCordsEditor (TuneCordsProcessor&);
    ~TuneCordsEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    struct Star { float x, y, size, phase, speed; };

    TuneCordsProcessor& processor;
    SkyLookAndFeel lnf;
    std::vector<Star> stars;
    juce::Image grain;
    double timeSec = 0.0;

    NoteSky sky;

    juce::ComboBox keyBox, scaleBox, voiceBox;
    juce::Label keyLabel, scaleLabel, voiceLabel;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    Knob amount, speed, tolerance, humanize, mix, output;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> keyAtt, scaleAtt, voiceAtt;

    void setupKnob (Knob&, const juce::String& id, const juce::String& title, const juce::String& suffix, int decimals);
    void setupCombo (juce::ComboBox&, juce::Label&, const juce::String& title);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TuneCordsEditor)
};
