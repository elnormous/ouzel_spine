// Copyright (C) 2016 Elviss Strazdins

#pragma once

class SpineSample: public ouzel::Noncopyable
{
public:
    virtual ~SpineSample();

    void run();

    bool handleKeyboard(ouzel::Event::Type type, const ouzel::KeyboardEvent& event) const;
    bool handleMouse(ouzel::Event::Type type, const ouzel::MouseEvent& event) const;
    bool handleTouch(ouzel::Event::Type type, const ouzel::TouchEvent& event) const;
    bool handleGamepad(ouzel::Event::Type type, const ouzel::GamepadEvent& event) const;
    bool handleUI(ouzel::Event::Type type, const ouzel::UIEvent& event) const;

protected:
    ouzel::scene::LayerPtr layer;

    ouzel::EventHandler eventHandler;
};
