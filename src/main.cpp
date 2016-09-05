// Copyright (C) 2015 Elviss Strazdins

#include "SpineSample.h"

ouzel::Engine engine;
SpineSample sample;

void ouzelMain(const std::vector<std::string>& args)
{
    OUZEL_UNUSED(args);

    ouzel::Settings settings;
    settings.size = ouzel::Size2(800.0f, 600.0f);
    settings.resizable = true;
    settings.sampleCount = 4;
    settings.textureFiltering = ouzel::graphics::Renderer::TextureFiltering::TRILINEAR;
    engine.init(settings);

    sample.run();
}
