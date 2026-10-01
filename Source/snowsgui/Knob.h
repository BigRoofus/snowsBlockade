#pragma once

#include "Theme.h"

namespace snowsgui
{
    class KnobSlider final : public juce::Slider
    {
    public:
        KnobSlider() : juce::Slider(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox) {}

        float bodyPixels = 100.0f;

        void paint(juce::Graphics& g) override
        {
            const auto c = getLocalBounds().toFloat().getCentre();
            const float u = bodyPixels / 100.0f;
            const auto angleOf = [](double proportion)
            {
                return juce::degreesToRadians(-135.0f + 270.0f * (float) proportion);
            };
            const float angle = angleOf(valueToProportionOfLength(getValue()));

            juce::ColourGradient halo(juce::Colours::black.withAlpha(0.5f), c,
                                      juce::Colours::transparentBlack, c.translated(58.0f * u, 0.0f), true);
            halo.addColour(0.6, juce::Colours::black.withAlpha(0.5f));
            g.setGradientFill(halo);
            g.fillEllipse(c.x - 58.0f * u, c.y - 58.0f * u, 116.0f * u, 116.0f * u);

            const auto strokeArc = [&](float from, float to, float width, juce::Colour colour)
            {
                juce::Path p;
                p.addCentredArc(c.x, c.y, 46.0f * u, 46.0f * u, 0.0f, from, to, true);
                g.setColour(colour);
                g.strokePath(p, juce::PathStrokeType(width * u, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            };

            strokeArc(angleOf(0.0), angleOf(1.0), 2.2f, juce::Colour(0xff454546));

            strokeArc(angleOf(0.0), juce::jmax(angle, angleOf(0.0) + juce::degreesToRadians(0.6f)), 2.6f, juce::Colour(0xfff1f0ec));

            juce::ColourGradient body(juce::Colour(0xff4b4b4c), c.x - 9.1f * u, c.y - 15.2f * u,
                                      juce::Colour(0xff252526), c.x - 9.1f * u + 64.6f * u, c.y - 15.2f * u, true);
            body.addColour(0.55, juce::Colour(0xff373738));
            g.setGradientFill(body);
            g.fillEllipse(c.x - 38.0f * u, c.y - 38.0f * u, 76.0f * u, 76.0f * u);
            g.setColour(juce::Colour(0xff505051));
            g.drawEllipse(c.x - 38.0f * u, c.y - 38.0f * u, 76.0f * u, 76.0f * u, 0.8f * u);

            const float dotX = c.x + 27.0f * u * std::sin(angle);
            const float dotY = c.y - 27.0f * u * std::cos(angle);
            g.setColour(ink);
            g.fillEllipse(dotX - 3.0f * u, dotY - 3.0f * u, 6.0f * u, 6.0f * u);
        }

        bool hitTest(int x, int y) override
        {
            return getLocalBounds().toFloat().getCentre().getDistanceFrom({ (float) x, (float) y }) <= 48.0f * bodyPixels / 100.0f;
        }
    };

    // A rotary knob with its value above and its name below.
    class Knob final : public juce::Component
    {
    public:
        struct Style
        {
            int width, valueHeight, gap, box, nameHeight;
            float valueFont, nameFont, nameKerning, bodyPixels;
            bool boldName;
        };

        static Style mediumStyle() { return { 84, 16, 4, 84, 14, 12.0f, 11.0f, 0.1f, 92.0f, false }; }
        static Style smallStyle() { return { 60, 13, 2, 52, 12, 11.0f, 10.0f, 0.1f, 56.0f, false }; }
        static Style mainStyle(float bodyPixels, bool boldName = false)
        {
            return { 216, 18, 6, 104, 22, 16.0f, 17.0f, 0.02f, bodyPixels, boldName };
        }

        // Room around the layout box for the knob's shadow halo.
        static constexpr int pad = 20;

        KnobSlider slider;
        std::function<void()> onChange;

        Knob(const juce::String& knobName, const Style& s, std::function<juce::String(double)> valueFormat)
            : name(knobName), style(s), format(std::move(valueFormat))
        {
            slider.bodyPixels = style.bodyPixels;
            slider.onValueChange = [this]
            {
                repaint();
                if (onChange)
                    onChange();
            };
            addAndMakeVisible(slider);
            setInterceptsMouseClicks(false, true);
        }

        // Positions the knob's layout box (not its halo padding) at x, y.
        void placeAt(int x, int y)
        {
            setBounds(x - pad, y - pad, style.width + 2 * pad, layoutHeight() + 2 * pad);
        }

        void resized() override
        {
            const int size = juce::roundToInt(style.bodyPixels * 1.16f) + 2;
            slider.setSize(size, size);
            slider.setCentrePosition(pad + style.width / 2, pad + style.valueHeight + style.gap + style.box / 2);
        }

        void paint(juce::Graphics& g) override
        {
            auto area = getLocalBounds().reduced(pad);
            g.setFont(font(style.valueFont));
            g.setColour(subtext);
            g.drawText(format(slider.getValue()), area.removeFromTop(style.valueHeight), juce::Justification::centred);

            g.setFont(font(style.nameFont, style.boldName, style.nameKerning));
            g.setColour(style.boldName ? ink : (style.width < 100 ? muted : text));
            g.drawText(name.toUpperCase(), area.removeFromBottom(style.nameHeight), juce::Justification::centred);
        }

    private:
        int layoutHeight() const { return style.valueHeight + style.gap + style.box + style.gap + style.nameHeight; }

        juce::String name;
        Style style;
        std::function<juce::String(double)> format;
    };
}
