// Copyright (C) 2017 Elviss Strazdins

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
    settings.textureFilter = ouzel::graphics::Texture::Filter::TRILINEAR;
    settings.title = "Spine import";
    engine.init(settings);

    sample.run();
}
