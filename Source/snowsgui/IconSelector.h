#pragma once

#include "Theme.h"

namespace snowsgui
{
    // A row of small buttons, one selected at a time, each drawn as an icon over its name.
    class IconSelector final : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        struct Option
        {
            juce::String name;
            juce::Path icon; // drawn inside a 32 x 20 box
        };

        static constexpr int buttonWidth = 64, buttonHeight = 44, gap = 10;

        std::function<void(int)> onSelect;

        explicit IconSelector(std::vector<Option> optionList) : options(std::move(optionList)) {}

        // A line icon of a function of x in [-1, 1] whose output stays within [-1, 1].
        static juce::Path curveIcon(const std::function<float(float)>& f)
        {
            constexpr int steps = 32;
            juce::Path path;
            for (int i = 0; i <= steps; ++i)
            {
                const float x = -1.0f + 2.0f * (float) i / steps;
                const juce::Point<float> p(16.0f + x * 13.0f, 10.0f - f(x) * 7.0f);
                if (i == 0)
                    path.startNewSubPath(p);
                else
                    path.lineTo(p);
            }
            return path;
        }

        int totalWidth() const
        {
            const int count = (int) options.size();
            return count * buttonWidth + juce::jmax(0, count - 1) * gap;
        }

        void setSelected(int index)
        {
            selected = index;
            repaint();
        }

        void paint(juce::Graphics& g) override
        {
            for (int i = 0; i < (int) options.size(); ++i)
            {
                const bool on = i == selected;
                const auto r = buttonBounds(i);

                g.setColour(on ? juce::Colour(0xff414142) : juce::Colour(0xff333334));
                g.fillRoundedRectangle(r, 4.0f);
                g.setColour(on ? juce::Colours::white.withAlpha(0.4f) : juce::Colours::white.withAlpha(0.07f));
                g.drawRoundedRectangle(r.reduced(0.5f), 4.0f, 1.0f);

                const auto tone = on ? juce::Colours::white : juce::Colour(0xffa8a6a1);
                const float top = r.getY() + 4.5f;

                g.setColour(tone);
                g.strokePath(options[(size_t) i].icon, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                             juce::AffineTransform::translation(r.getCentreX() - 16.0f, top));

                g.setFont(font(10.0f, false, 0.08f));
                g.drawText(options[(size_t) i].name.toUpperCase(), juce::Rectangle<float>(r.getX(), top + 23.0f, r.getWidth(), 12.0f),
                           juce::Justification::centred);
            }
        }

        void mouseDown(const juce::MouseEvent& e) override
        {
            for (int i = 0; i < (int) options.size(); ++i)
                if (buttonBounds(i).contains(e.position) && onSelect)
                    onSelect(i);
        }

    private:
        static juce::Rectangle<float> buttonBounds(int i)
        {
            return { (float) (i * (buttonWidth + gap)), 0.0f, (float) buttonWidth, (float) buttonHeight };
        }

        std::vector<Option> options;
        int selected = 0;
    };
}
