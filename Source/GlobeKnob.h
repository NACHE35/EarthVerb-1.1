#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include <vector>
#include <utility>

// Le gros bouton Dry/Wet : un globe terrestre qui tourne.
// Plus le Wet est élevé, plus la zone éclairée (jour) envahit le globe.
class GlobeKnob : public juce::Slider, private juce::Timer
{
public:
    GlobeKnob()
    {
        setSliderStyle (RotaryHorizontalVerticalDrag);
        setTextBoxStyle (NoTextBox, false, 0, 0);
        setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                             juce::MathConstants<float>::pi * 2.8f, true);
        setMouseDragSensitivity (250);
        buildLand();
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        using juce::Colour; using juce::ColourGradient;
        const auto b = getLocalBounds().toFloat();
        const auto c = b.getCentre();
        const float ringR = std::min (b.getWidth(), b.getHeight()) * 0.5f - 6.0f;
        const float r = ringR - 18.0f;
        const float mix = (float) valueToProportionOfLength (getValue());
        const float pi = juce::MathConstants<float>::pi;

        // Halo atmosphérique
        {
            const float R2 = ringR + 4.0f;
            ColourGradient halo (Colour (0x00000000), c.x, c.y, Colour (0x00000000), c.x + R2, c.y, true);
            halo.addColour ((double) (r / R2), Colour (0xff4fc3ff).withAlpha (0.10f + 0.45f * mix));
            g.setGradientFill (halo);
            g.fillEllipse (c.x - R2, c.y - R2, R2 * 2, R2 * 2);
        }

        // Anneau de valeur
        const auto rp = getRotaryParameters();
        const float a0 = rp.startAngleRadians, a1 = a0 + mix * (rp.endAngleRadians - rp.startAngleRadians);
        {
            const float ar = ringR - 4.0f;
            juce::Path track, val;
            track.addCentredArc (c.x, c.y, ar, ar, 0.0f, rp.startAngleRadians, rp.endAngleRadians, true);
            g.setColour (Colour (0xff1c2a44));
            g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            if (mix > 0.001f)
            {
                val.addCentredArc (c.x, c.y, ar, ar, 0.0f, a0, a1, true);
                g.setColour (Colour (0xff6fd0ff));
                g.strokePath (val, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
            g.setColour (Colours_white());
            g.fillEllipse (c.x + ar * std::sin (a1) - 5.0f, c.y - ar * std::cos (a1) - 5.0f, 10.0f, 10.0f);
        }

        // Globe
        juce::Path disc;
        disc.addEllipse (c.x - r, c.y - r, r * 2, r * 2);

        // lit region (jour) : demi-disque droit + ellipse terminateur, k = cos(mix*pi)
        juce::Path lit, terminator;
        {
            const float k = std::cos (mix * pi);
            const int N = 48;
            for (int i = 0; i <= N; ++i)
            {
                const float t = -pi / 2 + pi * (float) i / N;
                const float px = c.x + r * std::cos (t), py = c.y + r * std::sin (t);
                if (i == 0) lit.startNewSubPath (px, py); else lit.lineTo (px, py);
            }
            for (int i = N; i >= 0; --i)
            {
                const float t = -pi / 2 + pi * (float) i / N;
                const float px = c.x + k * r * std::cos (t), py = c.y + r * std::sin (t);
                lit.lineTo (px, py);
                if (i == N) terminator.startNewSubPath (px, py); else terminator.lineTo (px, py);
            }
            lit.closeSubPath();
        }

        // Continents projetés
        std::vector<juce::Path> land;
        for (auto& poly : continents)
        {
            juce::Path p;
            bool first = true;
            for (size_t i = 0; i < poly.size(); ++i)
            {
                const auto& A = poly[i];
                const auto& B = poly[(i + 1) % poly.size()];
                for (int s = 0; s < 4; ++s)
                {
                    const float f = (float) s / 4.0f;
                    const auto pt = project (A.first + (B.first - A.first) * f,
                                             A.second + (B.second - A.second) * f, c.x, c.y, r);
                    if (first) { p.startNewSubPath (pt); first = false; } else p.lineTo (pt);
                }
            }
            p.closeSubPath();
            land.push_back (p);
        }

        g.saveState();
        g.reduceClipRegion (disc, {});

        // NUIT
        g.setGradientFill (ColourGradient (Colour (0xff0d1c3d), c.x - r * 0.4f, c.y - r * 0.4f,
                                           Colour (0xff030612), c.x + r, c.y + r, true));
        g.fillPath (disc);
        for (auto& p : land)
        {
            g.setColour (Colour (0xff10263f)); g.fillPath (p);
            g.setColour (Colour (0xff3a6a9a).withAlpha (0.35f)); g.strokePath (p, juce::PathStrokeType (1.0f));
        }

        // JOUR
        g.saveState();
        g.reduceClipRegion (lit, {});
        g.setGradientFill (ColourGradient (Colour (0xff3aa0ee), c.x - r * 0.3f, c.y - r * 0.5f,
                                           Colour (0xff0a3e8c), c.x + r, c.y + r, true));
        g.fillPath (disc);
        for (size_t i = 0; i < land.size(); ++i)
        {
            g.setColour (i == landAntarctica ? Colour (0xffeaf4ff) : Colour (0xff4cae55));
            g.fillPath (land[i]);
            g.setColour (Colour (0xff2c7a3a)); g.strokePath (land[i], juce::PathStrokeType (1.0f));
        }
        g.restoreState();

        // Lueur du terminateur
        g.setColour (Colour (0xffffb35c).withAlpha (0.35f * std::sin (mix * pi) + 0.05f));
        g.strokePath (terminator, juce::PathStrokeType (5.0f));

        // Relief sphérique (vignette + reflet)
        g.setGradientFill (ColourGradient (Colour (0x00000000), c.x, c.y, Colour (0x99000000), c.x + r, c.y, true));
        g.fillPath (disc);
        g.setGradientFill (ColourGradient (Colour (0x30ffffff), c.x - r * 0.4f, c.y - r * 0.5f,
                                           Colour (0x00ffffff), c.x - r * 0.4f + r * 0.8f, c.y - r * 0.5f, true));
        g.fillPath (disc);
        g.restoreState();

        g.setColour (Colour (0xff9fd8ff).withAlpha (0.25f + 0.5f * mix));
        g.drawEllipse (c.x - r, c.y - r, r * 2, r * 2, 1.5f);
    }

private:
    static juce::Colour Colours_white() { return juce::Colour (0xffffffff); }

    void timerCallback() override { rotation += 0.006f; repaint(); }

    juce::Point<float> project (float lonDeg, float latDeg, float cx, float cy, float r) const
    {
        const float d2r = juce::MathConstants<float>::pi / 180.0f;
        const float lon = lonDeg * d2r - rotation, lat = latDeg * d2r;
        const float tilt = 0.38f;
        float x = std::cos (lat) * std::sin (lon);
        const float y = std::sin (lat);
        const float z = std::cos (lat) * std::cos (lon);
        float y2 = y * std::cos (tilt) - z * std::sin (tilt);
        const float z2 = y * std::sin (tilt) + z * std::cos (tilt);
        if (z2 < 0.0f)   // face cachée : on plaque sur le bord
        {
            float len = std::sqrt (x * x + y2 * y2);
            if (len < 1e-4f) { x = 1.0f; y2 = 0.0f; len = 1.0f; }
            x /= len; y2 /= len;
        }
        return { cx + x * r, cy - y2 * r };
    }

    void buildLand()
    {
        continents = {
            // Amérique du Nord
            { {-168,66},{-162,70},{-140,70},{-125,70},{-95,72},{-80,73},{-62,60},{-55,52},{-66,44},{-76,38},{-81,31},{-80,25},{-84,30},{-90,29},{-97,26},{-97,20},{-95,18},{-88,21},{-88,16},{-83,15},{-83,10},{-77,8},{-80,8},{-86,12},{-92,14},{-97,16},{-105,20},{-110,24},{-115,30},{-118,34},{-124,40},{-124,48},{-130,54},{-140,60},{-150,60},{-158,57},{-165,60} },
            // Amérique du Sud
            { {-77,8},{-72,12},{-62,10},{-52,5},{-50,0},{-44,-2},{-35,-6},{-39,-15},{-41,-22},{-48,-26},{-53,-34},{-58,-38},{-65,-41},{-66,-47},{-68,-53},{-72,-53},{-74,-45},{-73,-37},{-71,-28},{-70,-18},{-76,-14},{-81,-6},{-80,0},{-78,4} },
            // Afrique
            { {-17,21},{-10,30},{-6,35},{10,37},{11,33},{20,32},{32,31},{35,28},{43,12},{51,12},{48,4},{40,-5},{40,-15},{35,-24},{32,-29},{26,-34},{19,-35},{15,-27},{12,-17},{13,-8},{9,-1},{9,4},{4,6},{-8,4},{-13,8},{-17,14} },
            // Eurasie
            { {-10,36},{-9,43},{-2,44},{-4,48},{2,51},{8,54},{9,57},{11,56},{14,55},{20,55},{23,58},{30,60},{22,60},{21,65},{25,66},{18,63},{17,57},{11,59},{5,59},{5,62},{14,68},{25,71},{40,68},{45,68},{60,69},{70,73},{80,73},{100,77},{112,74},{130,71},{140,72},{160,70},{180,68},{180,65},{170,60},{163,58},{156,51},{155,58},{142,59},{135,54},{141,48},{132,43},{128,39},{126,35},{122,40},{119,37},{122,31},{120,25},{110,20},{107,17},{109,11},{105,9},{101,13},{100,7},{103,1},{98,8},{98,16},{94,17},{90,22},{86,20},{80,15},{77,8},{73,17},{70,21},{67,25},{57,25},{56,27},{50,30},{48,29},{52,24},{56,26},{60,22},{52,17},{43,13},{38,20},{35,28},{32,31},{35,36},{28,37},{26,40},{23,37},{20,40},{16,38},{12,42},{8,44},{3,43},{-1,37} },
            // Groenland
            { {-73,78},{-60,82},{-30,83},{-20,80},{-20,70},{-43,60},{-50,65},{-56,72} },
            // Australie
            { {114,-22},{122,-18},{130,-12},{136,-12},{142,-11},{146,-19},{153,-27},{150,-37},{141,-38},{135,-34},{129,-32},{115,-34},{114,-26} },
            // Îles britanniques, Japon, Madagascar, Sumatra
            { {-5,50},{1,51},{-2,57},{-5,58},{-6,55} },
            { {130,31},{135,34},{141,38},{142,44},{140,42},{136,37},{131,34} },
            { {44,-25},{49,-15},{50,-13},{44,-17} },
            { {95,5},{98,4},{106,-6},{104,-4} },
            // Antarctique (en dernier)
            { {-180,-72},{-150,-72},{-120,-74},{-90,-72},{-60,-68},{-30,-72},{0,-70},{30,-68},{60,-67},{90,-66},{120,-66},{150,-70},{180,-72},{180,-90},{-180,-90} }
        };
        landAntarctica = continents.size() - 1;
    }

    std::vector<std::vector<std::pair<float, float>>> continents;
    size_t landAntarctica = 0;
    float rotation = 0.0f;
};

// Look & feel des 4 petits boutons
class EarthLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startA, float endA, juce::Slider&) override
    {
        using juce::Colour;
        auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
        const float R = std::min (b.getWidth(), b.getHeight()) * 0.5f;
        const auto c = b.getCentre();
        const float ar = R - 2.0f, kr = R - 10.0f;
        const float a = startA + pos * (endA - startA);

        juce::Path track, val;
        track.addCentredArc (c.x, c.y, ar, ar, 0.0f, startA, endA, true);
        g.setColour (Colour (0xff1c2a44));
        g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        if (pos > 0.001f)
        {
            val.addCentredArc (c.x, c.y, ar, ar, 0.0f, startA, a, true);
            g.setColour (Colour (0xff6fd0ff));
            g.strokePath (val, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        g.setGradientFill (juce::ColourGradient (Colour (0xff34456b), c.x, c.y - kr, Colour (0xff0b1122), c.x, c.y + kr, false));
        g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
        g.setColour (Colour (0xff6fa8dc).withAlpha (0.4f));
        g.drawEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2, 1.0f);
        g.setColour (juce::Colours::white);
        g.drawLine (c.x + kr * 0.35f * std::sin (a), c.y - kr * 0.35f * std::cos (a),
                    c.x + kr * 0.9f * std::sin (a),  c.y - kr * 0.9f * std::cos (a), 2.5f);
    }
};
