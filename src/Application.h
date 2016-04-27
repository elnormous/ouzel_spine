// Copyright (C) 2016 Elviss Strazdins

#pragma once

class Application: public ouzel::Noncopyable
{
public:
    virtual ~Application();

    void begin();

    bool handleKeyboard(const ouzel::KeyboardEventPtr& event, const ouzel::VoidPtr& sender) const;
    bool handleMouse(const ouzel::MouseEventPtr& event, const ouzel::VoidPtr& sender) const;
    bool handleTouch(const ouzel::TouchEventPtr& event, const ouzel::VoidPtr& sender) const;
    bool handleGamepad(const ouzel::GamepadEventPtr& event, const ouzel::VoidPtr& sender) const;
    bool handleUI(const ouzel::UIEventPtr& event, const ouzel::VoidPtr& sender) const;

protected:
    ouzel::scene::LayerPtr layer;

    ouzel::EventHandlerPtr eventHandler;
};
