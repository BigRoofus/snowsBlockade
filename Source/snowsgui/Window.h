#pragma once

#include "Theme.h"
#include "SnowsGuiAssets.h"

namespace snowsgui
{
    // The fixed size a plugin's GUI is laid out at. The window scales uniformly from it.
    struct DesignSize
    {
        int width, height;
    };

    inline std::unique_ptr<juce::Drawable> createLogo()
    {
        return juce::Drawable::createFromImageData(snowsguiassets::snowlogo_svg, snowsguiassets::snowlogo_svgSize);
    }

    inline float scaleFor(const juce::Component& window, DesignSize design)
    {
        return juce::jmin((float) window.getWidth() / (float) design.width, (float) window.getHeight() / (float) design.height);
    }

    // Call at the end of the editor's constructor: lets the window resize from half to double
    // the design size, keeping its aspect ratio.
    inline void makeResizable(juce::AudioProcessorEditor& editor, DesignSize design)
    {
        editor.setResizable(true, true);
        editor.setResizeLimits(design.width / 2, design.height / 2, design.width * 2, design.height * 2);
        editor.getConstrainer()->setFixedAspectRatio((double) design.width / design.height);
        editor.setSize(design.width, design.height);
    }

    // Call from the editor's resized(). `content` holds every control at the design size.
    inline void fitContent(const juce::Component& window, juce::Component& content, DesignSize design)
    {
        content.setBounds(0, 0, design.width, design.height);
        content.setTransform(juce::AffineTransform::scale(scaleFor(window, design)));
    }

    // Call first in the editor's paint(): background, title, subtitle and corner logo, all
    // scaled to the window. Anything painted after it is in design coordinates.
    inline void paintBackdrop(juce::Graphics& g, const juce::Component& window, DesignSize design,
                              const juce::String& title, const juce::String& subtitle, const juce::Drawable* logo)
    {
        g.fillAll(canvas);
        g.addTransform(juce::AffineTransform::scale(scaleFor(window, design)));

        const auto wash = [&](float xFraction, float yFraction, float radius, juce::Colour colour)
        {
            g.setGradientFill(juce::ColourGradient(colour, xFraction * (float) design.width, yFraction * (float) design.height,
                                                   colour.withAlpha(0.0f), xFraction * (float) design.width + radius,
                                                   yFraction * (float) design.height, true));
            g.fillRect(0, 0, design.width, design.height);
        };
        wash(486.0f / 972.0f, 203.0f / 534.0f, 640.0f, washCentre);
        wash(108.0f / 972.0f, 480.0f / 534.0f, 360.0f, washCorner);
        wash(886.0f / 972.0f, 75.0f / 534.0f, 320.0f, washCorner);

        g.setFont(font(27.0f));
        g.setColour(ink);
        g.drawText(title.toUpperCase(), 0, 14, design.width, 32, juce::Justification::centred);

        g.setFont(font(16.0f));
        g.setColour(subtext);
        g.drawText(subtitle, 0, 46, design.width, 20, juce::Justification::centred);

        if (logo != nullptr)
            logo->drawWithin(g, juce::Rectangle<float>(16.0f, (float) design.height - 32.0f, 19.0f, 21.0f),
                             juce::RectanglePlacement::centred, 0.45f);
    }
}
