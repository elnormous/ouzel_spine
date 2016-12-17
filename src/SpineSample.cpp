// Copyright (C) 2016 Elviss Strazdins

#include <cmath>
#include "SpineSample.h"

using namespace std;
using namespace ouzel;

void SpineSample::run()
{
#if OUZEL_PLATFORM_LINUX
    sharedApplication->getFileSystem()->addResourcePath("Resources");
#elif OUZEL_PLATFORM_WINDOWS
    sharedApplication->getFileSystem()->addResourcePath("Resources");
    sharedApplication->getFileSystem()->addResourcePath("../Resources");
    sharedApplication->getFileSystem()->addResourcePath("../../Resources");
#endif

    eventHandler.keyboardHandler = std::bind(&SpineSample::handleKeyboard, this, std::placeholders::_1, std::placeholders::_2);
    eventHandler.mouseHandler = std::bind(&SpineSample::handleMouse, this, std::placeholders::_1, std::placeholders::_2);
    eventHandler.touchHandler = std::bind(&SpineSample::handleTouch, this, std::placeholders::_1, std::placeholders::_2);
    eventHandler.gamepadHandler = std::bind(&SpineSample::handleGamepad, this, std::placeholders::_1, std::placeholders::_2);
    eventHandler.uiHandler = std::bind(&SpineSample::handleUI, this, std::placeholders::_1, std::placeholders::_2);

    sharedEngine->getEventDispatcher()->addEventHandler(&eventHandler);

    sharedEngine->getRenderer()->setClearColor(Color(64, 0, 0));

    sharedEngine->getSceneManager()->setScene(&scene);
    layer.addCamera(&camera);
    scene.addLayer(&layer);

    witch.reset(new spine::SpineDrawable("witch.atlas", "witch.json"));

    witchNode.addComponent(witch.get());
    layer.addChild(&witchNode);

    witch->setAnimation(0, "walk", true);
    witch->addAnimation(0, "death", false, 2.0f);

    witch->setEventCallback([this](int32_t trackIndex, const spine::SpineDrawable::Event& event) {
        switch (event.type)
        {
            case spine::SpineDrawable::Event::Type::START:
                ouzel::Log(ouzel::Log::Level::INFO) << trackIndex << " start: " << witch->getAnimationName(trackIndex);
                break;
            case spine::SpineDrawable::Event::Type::END:
                ouzel::Log(ouzel::Log::Level::INFO) << trackIndex << " end: " << witch->getAnimationName(trackIndex);
                break;
            case spine::SpineDrawable::Event::Type::COMPLETE:
                ouzel::Log(ouzel::Log::Level::INFO) << trackIndex << " complete: " << witch->getAnimationName(trackIndex) << ", ";
                break;
            case spine::SpineDrawable::Event::Type::EVENT:
                ouzel::Log(ouzel::Log::Level::INFO) << trackIndex << " event: " << witch->getAnimationName(trackIndex) << ", " << event.name << ": " << event.intValue << ", " << event.floatValue << ", " << event.stringValue;
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
    if (type == ouzel::Event::Type::KEY_DOWN)
    {
        Vector2 position = camera.getPosition();

        switch (event.key)
        {
            case input::KeyboardKey::UP:
                position.y() += 10.0f;
                break;
            case input::KeyboardKey::DOWN:
                position.y() -= 10.0f;
                break;
            case input::KeyboardKey::LEFT:
                position.x() -= 10.0f;
                break;
            case input::KeyboardKey::RIGHT:
                position.x() += 10.0f;
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
        case ouzel::Event::Type::MOUSE_DOWN:
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
