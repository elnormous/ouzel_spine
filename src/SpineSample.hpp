// Copyright (C) 2017 Elviss Strazdins

#pragma once

#include "SpineDrawable.hpp"

class SpineSample: public ouzel::Noncopyable
{
public:
    SpineSample();

    bool handleKeyboard(ouzel::Event::Type type, const ouzel::KeyboardEvent& event);
    bool handleMouse(ouzel::Event::Type type, const ouzel::MouseEvent& event) const;
    bool handleTouch(ouzel::Event::Type type, const ouzel::TouchEvent& event) const;
    bool handleGamepad(ouzel::Event::Type type, const ouzel::GamepadEvent& event) const;
    bool handleUI(ouzel::Event::Type type, const ouzel::UIEvent& event) const;

protected:
    ouzel::scene::Layer layer;
    ouzel::scene::Camera camera;
    ouzel::scene::Scene scene;

    std::unique_ptr<spine::SpineDrawable> spineBoy;
    ouzel::scene::Node spineBoyNode;

    ouzel::EventHandler eventHandler;
};
