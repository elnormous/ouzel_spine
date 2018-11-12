// Copyright (C) 2017 Elviss Strazdins

#pragma once

#include "SpineDrawable.hpp"

class SpineSample: public ouzel::Application
{
public:
    SpineSample();

    bool handleKeyboard(const ouzel::KeyboardEvent& event);
    bool handleMouse(const ouzel::MouseEvent& event) const;
    bool handleTouch(const ouzel::TouchEvent& event) const;
    bool handleGamepad(const ouzel::GamepadEvent& event) const;
    bool handleUI(const ouzel::UIEvent& event) const;

protected:
    ouzel::scene::Layer layer;
    ouzel::scene::Actor cameraActor;
    ouzel::scene::Camera camera;
    ouzel::scene::Scene scene;
    ouzel::assets::Bundle bundle;

    std::unique_ptr<spine::SpineDrawable> spineBoy;
    ouzel::scene::Actor actor;

    ouzel::EventHandler eventHandler;
};
