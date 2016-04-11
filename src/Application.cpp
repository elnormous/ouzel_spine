// Copyright (C) 2016 Elviss Strazdins

#include <cmath>
#include "Application.h"
#include "spine-ouzel.h"

using namespace std;
using namespace ouzel;

Application::~Application()
{
    sharedEngine->getEventDispatcher()->removeEventHandler(_eventHandler);
}

void Application::begin()
{
    _eventHandler = make_shared<EventHandler>();

    _eventHandler->keyboardHandler = std::bind(&Application::handleKeyboard, this, std::placeholders::_1, std::placeholders::_2);
    _eventHandler->mouseHandler = std::bind(&Application::handleMouse, this, std::placeholders::_1, std::placeholders::_2);
    _eventHandler->touchHandler = std::bind(&Application::handleTouch, this, std::placeholders::_1, std::placeholders::_2);
    _eventHandler->gamepadHandler = std::bind(&Application::handleGamepad, this, std::placeholders::_1, std::placeholders::_2);
    _eventHandler->uiHandler = std::bind(&Application::handleUI, this, std::placeholders::_1, std::placeholders::_2);

    sharedEngine->getEventDispatcher()->addEventHandler(_eventHandler);

    sharedEngine->getRenderer()->setClearColor(video::Color(64, 0, 0));
    sharedEngine->getWindow()->setTitle("Spine import");

    scene::ScenePtr scene = make_shared<scene::Scene>();
    sharedEngine->getSceneManager()->setScene(scene);

    _layer = std::make_shared<scene::Layer>();

    ouzel::scene::CameraPtr camera = std::make_shared<scene::Camera>();
    _layer->setCamera(camera);
    scene->addLayer(_layer);

    std::shared_ptr<spine::SkeletonDrawable> drawable = std::make_shared<spine::SkeletonDrawable>("witch.atlas", "witch.json");
    _layer->addChild(drawable);

    //Slot* headSlot = Skeleton_findSlot(skeleton, "head");

    sharedEngine->getInput()->startGamepadDiscovery();
}

bool Application::handleKeyboard(const KeyboardEventPtr& event, const VoidPtr& sender) const
{
    if (event->type == ouzel::Event::Type::KEY_DOWN)
    {
        Vector2 position = _layer->getCamera()->getPosition();

        switch (event->key)
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

        _layer->getCamera()->setPosition(position);
    }

    return true;
}

bool Application::handleMouse(const MouseEventPtr& event, const VoidPtr& sender) const
{
    switch (event->type)
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

bool Application::handleTouch(const TouchEventPtr& event, const VoidPtr& sender) const
{
    return true;
}

bool Application::handleGamepad(const GamepadEventPtr& event, const VoidPtr& sender) const
{
    if (event->type == ouzel::Event::Type::GAMEPAD_BUTTON_CHANGE)
    {

    }

    return true;
}

bool Application::handleUI(const UIEventPtr& event, const VoidPtr& sender) const
{
    return true;
}
