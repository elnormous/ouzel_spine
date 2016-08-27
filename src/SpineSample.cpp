// Copyright (C) 2016 Elviss Strazdins

#include <cmath>
#include "SpineSample.h"
#include "SpineDrawable.h"

using namespace std;
using namespace ouzel;

SpineSample::~SpineSample()
{
    sharedEngine->getEventDispatcher()->removeEventHandler(eventHandler);
}

void SpineSample::run()
{
#if OUZEL_PLATFORM_LINUX
    sharedEngine->getFileSystem()->addResourcePath("Resources");
#elif OUZEL_PLATFORM_WINDOWS
    sharedEngine->getFileSystem()->addResourcePath("Resources");
    sharedEngine->getFileSystem()->addResourcePath("../Resources");
    sharedEngine->getFileSystem()->addResourcePath("../../Resources");
#endif

    eventHandler.keyboardHandler = std::bind(&SpineSample::handleKeyboard, this, std::placeholders::_1, std::placeholders::_2);
    eventHandler.mouseHandler = std::bind(&SpineSample::handleMouse, this, std::placeholders::_1, std::placeholders::_2);
    eventHandler.touchHandler = std::bind(&SpineSample::handleTouch, this, std::placeholders::_1, std::placeholders::_2);
    eventHandler.gamepadHandler = std::bind(&SpineSample::handleGamepad, this, std::placeholders::_1, std::placeholders::_2);
    eventHandler.uiHandler = std::bind(&SpineSample::handleUI, this, std::placeholders::_1, std::placeholders::_2);

    sharedEngine->getEventDispatcher()->addEventHandler(eventHandler);

    sharedEngine->getRenderer()->setClearColor(graphics::Color(64, 0, 0));
    sharedEngine->getWindow()->setTitle("Spine import");

    scene::ScenePtr scene = make_shared<scene::Scene>();
    sharedEngine->getSceneManager()->setScene(scene);

    layer = std::make_shared<scene::Layer>();

    ouzel::scene::CameraPtr camera = std::make_shared<scene::Camera>();
    layer->setCamera(camera);
    scene->addLayer(layer);

    std::shared_ptr<spine::SpineDrawable> witch = std::make_shared<spine::SpineDrawable>("witch.atlas", "witch.json");

    ouzel::scene::NodePtr witchNode = std::make_shared<ouzel::scene::Node>();
    witchNode->addComponent(witch);
    layer->addChild(witchNode);

    witch->setAnimation(0, "walk", true);
    witch->addAnimation(0, "death", false, 2.0f);

    witch->setEventCallback([witch](int trackIndex, spEventType type, spEvent* event, int loopCount) {
        spTrackEntry* entry = spAnimationState_getCurrent(witch->getAnimationState(), trackIndex);
        const char* animationName = (entry && entry->animation) ? entry->animation->name : nullptr;

        switch (type)
        {
            case SP_ANIMATION_START:
                log("%d start: %s", trackIndex, animationName);
                break;
            case SP_ANIMATION_END:
                log("%d end: %s", trackIndex, animationName);
                break;
            case SP_ANIMATION_COMPLETE:
                log("%d complete: %s, %d", trackIndex, animationName, loopCount);
                break;
            case SP_ANIMATION_EVENT:
                log("%d event: %s, %s: %d, %f, %s", trackIndex, animationName, event->data->name, event->intValue, event->floatValue, event->stringValue);
                break;
        }
    });

    //Slot* headSlot = Skeleton_findSlot(skeleton, "head");

    sharedEngine->getInput()->startGamepadDiscovery();
}

bool SpineSample::handleKeyboard(Event::Type type, const KeyboardEvent& event) const
{
    if (type == ouzel::Event::Type::KEY_DOWN)
    {
        Vector2 position = layer->getCamera()->getPosition();

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

        layer->getCamera()->setPosition(position);
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
