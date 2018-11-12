// Copyright (C) 2017 Elviss Strazdins

#include "SpineSample.hpp"

std::unique_ptr<ouzel::Application> ouzel::main(const std::vector<std::string>&)
{
    return std::unique_ptr<Application>(new SpineSample());
}
