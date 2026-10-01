#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace snowsgui
{
    // Text colours, brightest to dimmest.
    const juce::Colour ink     { 0xfff4f3ef };
    const juce::Colour text    { 0xffecebe8 };
    const juce::Colour subtext { 0xffcfcdc8 };
    const juce::Colour muted   { 0xff9b9995 };

    // Window background: a flat canvas with three soft lighter washes on top.
    const juce::Colour canvas     { 0xff262627 };
    const juce::Colour washCentre { 0xff343435 };
    const juce::Colour washCorner { 0xff2f2f30 };

    // Sizes are given in the design's CSS pixels; JUCE font heights include descenders.
    inline juce::Font font(float cssPixels, bool bold = false, float kerning = 0.0f)
    {
        return juce::Font("Segoe UI", cssPixels * 1.3f, bold ? juce::Font::bold : juce::Font::plain)
            .withExtraKerningFactor(kerning);
    }

    inline juce::String formatHz(double f)
    {
        if (f < 1000.0)
            return juce::String(juce::roundToInt(f)) + " Hz";

        auto s = juce::String(f / 1000.0, 1);
        if (s.endsWith(".0"))
            s = s.dropLastCharacters(2);
        return s + " kHz";
    }

    inline juce::String formatDecimal(double v)
    {
        return juce::String(v, 2);
    }

    inline juce::String formatPercent(double v)
    {
        return juce::String(juce::roundToInt(v)) + " %";
    }

    // Darkens the top edge of a rounded rectangle, like an inset well.
    inline void innerShadow(juce::Graphics& g, juce::Rectangle<float> r, float corner, float alpha, float depth)
    {
        juce::Graphics::ScopedSaveState state(g);
        juce::Path clip;
        clip.addRoundedRectangle(r, corner);
        g.reduceClipRegion(clip);
        g.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(alpha), r.getX(), r.getY(),
                                               juce::Colours::transparentBlack, r.getX(), r.getY() + depth, false));
        g.fillRect(r);
    }
}
