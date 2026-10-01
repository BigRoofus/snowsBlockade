#pragma once

#include "Theme.h"

namespace snowsgui
{
    class TextToggle final : public juce::ToggleButton
    {
    public:
        using juce::ToggleButton::ToggleButton;

        void paintButton(juce::Graphics& g, bool, bool) override
        {
            const bool on = getToggleState();
            const auto r = getLocalBounds().toFloat();

            if (on)
            {
                g.setColour(juce::Colour(0xffa9a8a3));
                g.fillRoundedRectangle(r.withTrimmedTop(2.0f), 3.0f);
                g.setColour(juce::Colour(0xfff1f0ec));
                g.fillRoundedRectangle(r.withTrimmedBottom(2.0f), 3.0f);
            }
            else
            {
                g.setColour(juce::Colour(0xff343435));
                g.fillRoundedRectangle(r, 3.0f);
                g.setColour(juce::Colour(0xff4b4b4c));
                g.drawRoundedRectangle(r.reduced(0.5f), 3.0f, 1.0f);
            }

            g.setFont(font(14.0f, false, 0.06f));
            g.setColour(on ? juce::Colour(0xff1e1e1f) : juce::Colour(0xffb7b5b0));
            g.drawText(getButtonText().toUpperCase(), r.withTrimmedBottom(on ? 2.0f : 0.0f), juce::Justification::centred);
        }
    };

    // On/off pill with its state above and its name below, sized like a main knob.
    class PillSwitch final : public juce::ToggleButton
    {
    public:
        using juce::ToggleButton::ToggleButton;

        bool hitTest(int x, int y) override { return pill().contains(x, y); }

        void paintButton(juce::Graphics& g, bool, bool) override
        {
            const bool on = getToggleState();

            g.setFont(font(16.0f));
            g.setColour(subtext);
            g.drawText(on ? "ON" : "OFF", 0, 0, getWidth(), 18, juce::Justification::centred);

            const auto track = pill().toFloat();
            g.setColour(juce::Colour(0xff171718));
            g.fillRoundedRectangle(track, track.getHeight() / 2.0f);
            innerShadow(g, track, track.getHeight() / 2.0f, 0.75f, 9.0f);

            const auto dot = juce::Rectangle<float>(24.0f, 24.0f).withCentre({ on ? track.getRight() - 17.0f : track.getX() + 17.0f,
                                                                               track.getCentreY() });
            if (on)
            {
                for (float grow : { 6.0f, 4.0f, 2.0f })
                {
                    g.setColour(ink.withAlpha(0.06f));
                    g.fillEllipse(dot.expanded(grow));
                }
            }
            g.setColour(on ? ink : juce::Colour(0xff6f6d72));
            g.fillEllipse(dot);

            g.setFont(font(17.0f, false, 0.02f));
            g.setColour(snowsgui::text);
            g.drawText(getButtonText().toUpperCase(), 0, getHeight() - 22, getWidth(), 22, juce::Justification::centred);
        }

    private:
        juce::Rectangle<int> pill() const
        {
            return juce::Rectangle<int>(72, 34).withCentre({ getWidth() / 2, 18 + 6 + 52 });
        }
    };
}
