// Copyright (C) 2017 Elviss Strazdins

#include <cmath>
#include "SpineSample.hpp"

using namespace std;
using namespace ouzel;

SpineSample::SpineSample()
{
#if OUZEL_PLATFORM_LINUX
    sharedEngine->getFileSystem()->addResourcePath("Resources");
#elif OUZEL_PLATFORM_WINDOWS
    sharedEngine->getFileSystem()->addResourcePath("Resources");
#endif

    eventHandler.keyboardHandler = std::bind(&SpineSample::handleKeyboard, this, std::placeholders::_1, std::placeholders::_2);
    eventHandler.mouseHandler = std::bind(&SpineSample::handleMouse, this, std::placeholders::_1, std::placeholders::_2);
    eventHandler.touchHandler = std::bind(&SpineSample::handleTouch, this, std::placeholders::_1, std::placeholders::_2);
    eventHandler.gamepadHandler = std::bind(&SpineSample::handleGamepad, this, std::placeholders::_1, std::placeholders::_2);
    eventHandler.uiHandler = std::bind(&SpineSample::handleUI, this, std::placeholders::_1, std::placeholders::_2);

    sharedEngine->getEventDispatcher()->addEventHandler(&eventHandler);

    sharedEngine->getRenderer()->setClearColor(Color(64, 0, 0));

    sharedEngine->getSceneManager()->setScene(&scene);

    layer.addChild(&camera);
    scene.addLayer(&layer);

    spineBoy.reset(new spine::SpineDrawable("spineboy.atlas", "spineboy.skel"));

    spineBoyNode.addComponent(spineBoy.get());
    spineBoyNode.setPosition({0, -100});
    spineBoyNode.setScale(Vector2(0.4f, 0.4f));
    layer.addChild(&spineBoyNode);

    spineBoy->setAnimation(0, "jump", false);
    spineBoy->addAnimation(0, "run", true, 2.0f);

    spineBoy->setEventCallback([this](int32_t trackIndex, const spine::SpineDrawable::Event& event) {
        switch (event.type)
        {
            case spine::SpineDrawable::Event::Type::START:
                ouzel::Log(ouzel::Log::Level::INFO) << trackIndex << " start: " << spineBoy->getAnimationName(trackIndex);
                break;
            case spine::SpineDrawable::Event::Type::END:
                ouzel::Log(ouzel::Log::Level::INFO) << trackIndex << " end: " << spineBoy->getAnimationName(trackIndex);
                break;
            case spine::SpineDrawable::Event::Type::COMPLETE:
                ouzel::Log(ouzel::Log::Level::INFO) << trackIndex << " complete: " << spineBoy->getAnimationName(trackIndex) << ", ";
                break;
            case spine::SpineDrawable::Event::Type::EVENT:
                ouzel::Log(ouzel::Log::Level::INFO) << trackIndex << " event: " << spineBoy->getAnimationName(trackIndex) << ", " << event.name << ": " << event.intValue << ", " << event.floatValue << ", " << event.stringValue;
                break;
            default:
                break;
        }
    });

    //Slot* headSlot = Skeleton_findSlot(skeleton, "head");

    sharedEngine->getInput()->startGamepadDiscovery();
}

bool SpineSample::handleKeyboard(Event::Type type, const KeyboardEvent& event)
{
    if (type == ouzel::Event::Type::KEY_PRESS)
    {
        Vector2 position = camera.getPosition();

        switch (event.key)
        {
            case input::KeyboardKey::UP:
                position.y += 10.0f;
                break;
            case input::KeyboardKey::DOWN:
                position.y -= 10.0f;
                break;
            case input::KeyboardKey::LEFT:
                position.x -= 10.0f;
                break;
            case input::KeyboardKey::RIGHT:
                position.x += 10.0f;
                break;
            case input::KeyboardKey::RETURN:
                sharedEngine->getWindow()->setSize(Size2(640.0f, 480.0f));
                break;
            default:
                break;
        }

        camera.setPosition(position);
    }

    return true;
}

bool SpineSample::handleMouse(Event::Type type, const MouseEvent& event) const
{
    switch (type)
    {
        case ouzel::Event::Type::MOUSE_PRESS:
        {
            break;
        }
        case ouzel::Event::Type::MOUSE_MOVE:
        {
            break;
        }
        default:
            break;
    }

    return true;
}

bool SpineSample::handleTouch(Event::Type type, const TouchEvent& event) const
{
    return true;
}

bool SpineSample::handleGamepad(Event::Type type, const GamepadEvent& event) const
{
    if (type == ouzel::Event::Type::GAMEPAD_BUTTON_CHANGE)
    {

    }

    return true;
}

bool SpineSample::handleUI(Event::Type type, const UIEvent& event) const
{
    return true;
}
