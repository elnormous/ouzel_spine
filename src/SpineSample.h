// Copyright (C) 2017 Elviss Strazdins

#pragma once

#include "SpineDrawable.h"

class SpineSample: public ouzel::Noncopyable
{
public:
    void run();

    bool handleKeyboard(ouzel::Event::Type type, const ouzel::KeyboardEvent& event);
    bool handleMouse(ouzel::Event::Type type, const ouzel::MouseEvent& event) const;
    bool handleTouch(ouzel::Event::Type type, const ouzel::TouchEvent& event) const;
    bool handleGamepad(ouzel::Event::Type type, const ouzel::GamepadEvent& event) const;
    bool handleUI(ouzel::Event::Type type, const ouzel::UIEvent& event) const;

protected:
    std::shared_ptr<ouzel::scene::Layer> layer;
    std::shared_ptr<ouzel::scene::Camera> camera;
    std::shared_ptr<ouzel::scene::Scene> scene;

    std::shared_ptr<spine::SpineDrawable> witch;
    std::shared_ptr<ouzel::scene::Node> witchNode;

    ouzel::EventHandler eventHandler;
};
