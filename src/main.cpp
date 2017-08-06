// Copyright (C) 2017 Elviss Strazdins

#include "SpineSample.hpp"

std::string DEVELOPER_NAME = "elnormous";
std::string APPLICATION_NAME = "Spine sample";

std::unique_ptr<SpineSample> sample;

void ouzelMain(const std::vector<std::string>& args)
{
    OUZEL_UNUSED(args);

    sample.reset(new SpineSample());
}
