#include "PluginEditor.h"

namespace
{
    const juce::Colour cream   (0xfff3e9d2);
    const juce::Colour gold    (0xffe8c77d);
    const juce::Colour amber   (0xffffb86b);
    const juce::Colour iceBlue (0xff9ad7ff);
    const juce::Colour dimStar (0xff55618a);

    juce::String noteName (int pc)
    {
        static const char* names[12] = { "Do", "Do#", "R\xc3\xa9", "R\xc3\xa9#", "Mi", "Fa",
                                         "Fa#", "Sol", "Sol#", "La", "La#", "Si" };
        return juce::String::fromUTF8 (names[((pc % 12) + 12) % 12]);
    }

    juce::String noteWithOctave (int midi)
    {
        return noteName (midi) + juce::String (midi / 12 - 1);
    }
}

//==============================================================================
SkyLookAndFeel::SkyLookAndFeel()
{
    setColour (juce::ComboBox::textColourId, cream);
    setColour (juce::ComboBox::arrowColourId, gold);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff0b1030));
    setColour (juce::PopupMenu::textColourId, cream);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff2a2f66));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::Label::textColourId, cream);
    setColour (juce::Slider::textBoxTextColourId, cream);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

juce::Font SkyLookAndFeel::serif (float size, bool italic)
{
    return juce::Font (juce::FontOptions ("Georgia", size, italic ? juce::Font::italic : juce::Font::plain));
}

juce::Font SkyLookAndFeel::getComboBoxFont (juce::ComboBox&) { return serif (15.0f); }
juce::Font SkyLookAndFeel::getLabelFont (juce::Label& l)     { return serif (l.getFont().getHeight()); }

void SkyLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (10, 1, box.getWidth() - 30, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

void SkyLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox&)
{
    auto r = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (0.5f);
    g.setColour (juce::Colour (0x66060a1c));
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (juce::Colour (0x55e8c77d));
    g.drawRoundedRectangle (r, 8.0f, 1.0f);

    juce::Path arrow;
    const float cx = (float) width - 14.0f, cy = (float) height * 0.5f;
    arrow.addTriangle (cx - 4.5f, cy - 2.0f, cx + 4.5f, cy - 2.0f, cx, cy + 3.5f);
    g.setColour (gold);
    g.fillPath (arrow);
}

void SkyLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float pos,
                                       float startAngle, float endAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (10.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);

    // petites étoiles repères autour du bouton
    for (int i = 0; i <= 10; ++i)
    {
        float a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
        float rr = radius + 6.0f;
        float px = c.x + std::sin (a) * rr, py = c.y - std::cos (a) * rr;
        bool lit = (float) i / 10.0f <= pos + 0.001f;
        g.setColour (lit ? gold : dimStar.withAlpha (0.7f));
        float s = (i % 5 == 0) ? 2.6f : 1.6f;
        g.fillEllipse (px - s * 0.5f, py - s * 0.5f, s, s);
    }

    // arc de valeur (halo doré)
    juce::Path arc;
    arc.addCentredArc (c.x, c.y, radius + 1.5f, radius + 1.5f, 0.0f, startAngle, angle, true);
    g.setColour (gold.withAlpha (0.18f));
    g.strokePath (arc, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (gold.withAlpha (0.85f));
    g.strokePath (arc, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // corps du bouton : métal sombre
    const float kr = radius * 0.86f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a4160), c.x - kr * 0.5f, c.y - kr * 0.7f,
                                             juce::Colour (0xff070914), c.x + kr * 0.7f, c.y + kr * 0.9f, true));
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);

    // stries concentriques, façon aluminium brossé
    g.setColour (juce::Colours::white.withAlpha (0.045f));
    for (float rr = kr * 0.25f; rr < kr; rr += 2.6f)
        g.drawEllipse (c.x - rr, c.y - rr, rr * 2.0f, rr * 2.0f, 0.6f);

    g.setColour (juce::Colour (0x66e8c77d));
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f, 1.0f);

    // reflet
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.16f), c.x - kr * 0.3f, c.y - kr * 0.8f,
                                             juce::Colours::transparentWhite, c.x, c.y - kr * 0.1f, false));
    g.fillEllipse (c.x - kr * 0.8f, c.y - kr * 0.95f, kr * 1.6f, kr * 1.0f);

    // aiguille
    juce::Path needle;
    needle.addRoundedRectangle (-1.4f, -kr * 0.92f, 2.8f, kr * 0.46f, 1.4f);
    g.setColour (cream);
    g.fillPath (needle, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
}

//==============================================================================
void NoteSky::tick()
{
    const float m = corrector.detectedMidi.load();
    if (m >= 0.0f)
    {
        float pc = std::fmod (m, 12.0f);
        shownPc = (shownPc < 0.0f) ? pc : shownPc + 0.5f * (pc - shownPc);
        trail[trailPos] = shownPc; trailValid[trailPos] = true;
    }
    else
    {
        shownPc = -1.0f;
        trailValid[trailPos] = false;
    }
    trailPos = (trailPos + 1) % 24;
    repaint();
}

void NoteSky::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0x40060a1c));
    g.fillRoundedRectangle (r, 14.0f);
    g.setColour (juce::Colour (0x30f3e9d2));
    g.drawRoundedRectangle (r.reduced (0.5f), 14.0f, 1.0f);

    const int root = (int) *apvts.getRawParameterValue ("key");
    const int mask = tc::scaleMask ((int) *apvts.getRawParameterValue ("scale"));

    const float left = 28.0f, width = r.getWidth() - 56.0f;
    const float baseY = r.getHeight() * 0.62f, arcH = r.getHeight() * 0.30f;
    auto starPos = [&] (float pc)
    {
        float t = (pc + 0.5f) / 12.0f;
        float x = left + t * width;
        float y = baseY - arcH * std::sin (juce::MathConstants<float>::pi * t);
        return juce::Point<float> (x, y);
    };

    // arc discret
    juce::Path arcPath;
    for (int i = 0; i <= 60; ++i)
    {
        auto p = starPos (-0.5f + 12.0f * (float) i / 60.0f);
        if (i == 0) arcPath.startNewSubPath (p); else arcPath.lineTo (p);
    }
    g.setColour (juce::Colour (0x22f3e9d2));
    g.strokePath (arcPath, juce::PathStrokeType (1.0f));

    const int target = (int) std::lround (corrector.targetMidiOut.load());
    const bool hasTarget = corrector.targetMidiOut.load() >= 0.0f;

    for (int pc = 0; pc < 12; ++pc)
    {
        const bool inScale = tc::noteInScale (pc, root, mask);
        auto p = starPos ((float) pc);

        if (inScale)
        {
            g.setColour (cream.withAlpha (0.10f));
            g.fillEllipse (p.x - 9.0f, p.y - 9.0f, 18.0f, 18.0f);
            g.setColour (cream);
            g.fillEllipse (p.x - 3.4f, p.y - 3.4f, 6.8f, 6.8f);
            g.setColour (cream.withAlpha (0.6f));
            g.drawLine (p.x - 7.0f, p.y, p.x + 7.0f, p.y, 0.8f);
            g.drawLine (p.x, p.y - 7.0f, p.x, p.y + 7.0f, 0.8f);
            g.setFont (SkyLookAndFeel::serif (12.0f));
            g.setColour (cream.withAlpha (0.85f));
            g.drawText (noteName (pc), juce::Rectangle<float> (p.x - 22.0f, p.y + 11.0f, 44.0f, 16.0f),
                        juce::Justification::centred);
        }
        else
        {
            g.setColour (dimStar);
            g.fillEllipse (p.x - 1.6f, p.y - 1.6f, 3.2f, 3.2f);
        }

        if (hasTarget && ((target % 12) + 12) % 12 == pc)
        {
            g.setColour (iceBlue.withAlpha (0.9f));
            g.drawEllipse (p.x - 12.0f, p.y - 12.0f, 24.0f, 24.0f, 1.3f);
        }
    }

    // traînée + étoile de la voix
    for (int i = 0; i < 24; ++i)
    {
        int idx = (trailPos + i) % 24;
        if (!trailValid[idx]) continue;
        auto p = starPos (trail[idx]);
        float a = (float) (i + 1) / 24.0f;
        g.setColour (amber.withAlpha (0.35f * a));
        float s = 2.0f + 3.0f * a;
        g.fillEllipse (p.x - s * 0.5f, p.y - s * 0.5f, s, s);
    }
    if (shownPc >= 0.0f)
    {
        auto p = starPos (shownPc);
        g.setColour (amber.withAlpha (0.22f));
        g.fillEllipse (p.x - 12.0f, p.y - 12.0f, 24.0f, 24.0f);
        g.setColour (amber);
        g.fillEllipse (p.x - 4.5f, p.y - 4.5f, 9.0f, 9.0f);
    }

    // texte d'état
    g.setFont (SkyLookAndFeel::serif (14.0f, true));
    const float m = corrector.detectedMidi.load();
    juce::String text;
    if (m < 0.0f)
        text = juce::String::fromUTF8 ("en attente d'une voix...");
    else
    {
        int nearest = (int) std::lround (m);
        int cents = (int) std::lround ((m - (float) nearest) * 100.0f);
        text = juce::String::fromUTF8 ("voix  ") + noteWithOctave (nearest)
               + (cents >= 0 ? " +" : " ") + juce::String (cents) + " c";
        if (hasTarget) text += juce::String::fromUTF8 ("   \xe2\x86\x92   cible  ") + noteWithOctave (target);
    }
    g.setColour (cream.withAlpha (0.8f));
    g.drawText (text, r.removeFromBottom (24.0f), juce::Justification::centred);
}

//==============================================================================
TuneCordsEditor::TuneCordsEditor (TuneCordsProcessor& p)
    : AudioProcessorEditor (&p), processor (p), sky (p.corrector, p.apvts)
{
    setLookAndFeel (&lnf);
    setSize (700, 450);

    juce::Random rnd (42);
    for (int i = 0; i < 130; ++i)
        stars.push_back ({ rnd.nextFloat(), rnd.nextFloat() * 0.92f,
                           0.6f + rnd.nextFloat() * rnd.nextFloat() * 2.6f,
                           rnd.nextFloat() * 6.28f, 0.4f + rnd.nextFloat() * 1.8f });

    grain = juce::Image (juce::Image::ARGB, 256, 256, true);
    for (int yy = 0; yy < 256; ++yy)
        for (int xx = 0; xx < 256; ++xx)
            grain.setPixelAt (xx, yy, juce::Colour::fromRGBA (255, 245, 225, (juce::uint8) rnd.nextInt (14)));

    setupCombo (keyBox,   keyLabel,   juce::String::fromUTF8 ("TONALIT\xc3\x89"));
    setupCombo (scaleBox, scaleLabel, "GAMME");
    setupCombo (voiceBox, voiceLabel, "VOIX");

    keyBox.addItemList (processor.apvts.getParameter ("key")->getAllValueStrings(), 1);
    scaleBox.addItemList (processor.apvts.getParameter ("scale")->getAllValueStrings(), 1);
    voiceBox.addItemList (processor.apvts.getParameter ("voice")->getAllValueStrings(), 1);
    keyAtt   = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "key", keyBox);
    scaleAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "scale", scaleBox);
    voiceAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "voice", voiceBox);

    addAndMakeVisible (sky);

    setupKnob (amount,    "amount",    "PUISSANCE", " %", 0);
    setupKnob (speed,     "speed",     "VITESSE", " ms", 0);
    setupKnob (tolerance, "tolerance", juce::String::fromUTF8 ("TOL\xc3\x89RANCE"), " c", 0);
    setupKnob (humanize,  "humanize",  "HUMANIZE", " %", 0);
    setupKnob (mix,       "mix",       "MIX", " %", 0);
    setupKnob (output,    "output",    "SORTIE", " dB", 1);

    startTimerHz (30);
}

TuneCordsEditor::~TuneCordsEditor()
{
    setLookAndFeel (nullptr);
}

void TuneCordsEditor::setupCombo (juce::ComboBox& box, juce::Label& label, const juce::String& title)
{
    addAndMakeVisible (box);
    label.setText (title, juce::dontSendNotification);
    label.setFont (SkyLookAndFeel::serif (11.0f));
    label.setColour (juce::Label::textColourId, gold.withAlpha (0.85f));
    label.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (label);
}

void TuneCordsEditor::setupKnob (Knob& k, const juce::String& id, const juce::String& title,
                                 const juce::String& suffix, int decimals)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
    k.slider.setTextValueSuffix (suffix);
    k.slider.setNumDecimalPlacesToDisplay (decimals);
    k.slider.setDoubleClickReturnValue (true, processor.apvts.getParameter (id)->convertFrom0to1 (
                                                  processor.apvts.getParameter (id)->getDefaultValue()));
    addAndMakeVisible (k.slider);
    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, id, k.slider);

    k.label.setText (title, juce::dontSendNotification);
    k.label.setFont (SkyLookAndFeel::serif (12.0f));
    k.label.setColour (juce::Label::textColourId, gold.withAlpha (0.9f));
    k.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (k.label);
}

void TuneCordsEditor::timerCallback()
{
    timeSec += 1.0 / 30.0;
    sky.tick();
    repaint();
}

void TuneCordsEditor::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();

    // ciel : nuit profonde -> bleu indigo -> lueur chaude à l'horizon
    juce::ColourGradient sky1 (juce::Colour (0xff03050c), 0, 0, juce::Colour (0xff2b1d4a), 0, b.getHeight(), false);
    sky1.addColour (0.55, juce::Colour (0xff0b1236));
    sky1.addColour (0.85, juce::Colour (0xff1c1a45));
    g.setGradientFill (sky1);
    g.fillAll();

    g.setGradientFill (juce::ColourGradient (juce::Colour (0x40ff9a6b), b.getCentreX(), b.getHeight() + 40.0f,
                                             juce::Colours::transparentBlack, b.getCentreX(), b.getHeight() - 220.0f, true));
    g.fillRect (b);

    // étoiles qui scintillent
    for (const auto& s : stars)
    {
        float tw = 0.5f + 0.5f * std::sin ((float) timeSec * s.speed + s.phase);
        float a = 0.25f + 0.75f * tw;
        float px = s.x * b.getWidth(), py = s.y * b.getHeight();
        g.setColour (cream.withAlpha (a * 0.9f));
        g.fillEllipse (px - s.size * 0.5f, py - s.size * 0.5f, s.size, s.size);
        if (s.size > 2.2f)
        {
            g.setColour (cream.withAlpha (a * 0.35f));
            g.drawLine (px - s.size * 2.2f, py, px + s.size * 2.2f, py, 0.7f);
            g.drawLine (px, py - s.size * 2.2f, px, py + s.size * 2.2f, 0.7f);
        }
    }

    // titre
    g.setColour (cream);
    g.setFont (SkyLookAndFeel::serif (34.0f));
    g.drawText ("T U N E   C O R D S", 30, 14, 440, 42, juce::Justification::centredLeft);
    g.setColour (gold.withAlpha (0.8f));
    g.setFont (SkyLookAndFeel::serif (12.5f, true));
    g.drawText (juce::String::fromUTF8 ("correction de hauteur  \xc2\xb7  ciel analogique"),
                32, 52, 440, 18, juce::Justification::centredLeft);

    // grain analogique
    g.setTiledImageFill (grain, 0, 0, 1.0f);
    g.fillAll();

    // liseré de cadre
    g.setColour (juce::Colour (0x33e8c77d));
    g.drawRoundedRectangle (b.reduced (6.0f), 10.0f, 1.0f);
}

void TuneCordsEditor::resized()
{
    const int m = 30;
    const int rowY = 78;
    auto place = [&] (juce::Label& l, juce::ComboBox& c, int x, int w)
    {
        l.setBounds (x + 4, rowY, w, 14);
        c.setBounds (x, rowY + 15, w, 28);
    };
    place (keyLabel,   keyBox,   m, 130);
    place (scaleLabel, scaleBox, m + 145, 210);
    place (voiceLabel, voiceBox, m + 370, 130);

    sky.setBounds (m, 130, getWidth() - 2 * m, 150);

    Knob* knobs[6] = { &amount, &speed, &tolerance, &humanize, &mix, &output };
    const int cellW = (getWidth() - 2 * m) / 6;
    for (int i = 0; i < 6; ++i)
    {
        int x = m + i * cellW;
        knobs[i]->label.setBounds (x, 296, cellW, 16);
        knobs[i]->slider.setBounds (x + 4, 312, cellW - 8, 124);
    }
}
