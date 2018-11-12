// Copyright (C) 2017 Elviss Strazdins

#include <cmath>
#include "SpineSample.hpp"

using namespace std;
using namespace ouzel;

SpineSample::SpineSample():
    bundle(engine->getCache())
{
#if OUZEL_PLATFORM_LINUX
    engine->getFileSystem().addResourcePath("Resources");
#elif OUZEL_PLATFORM_WINDOWS
    engine->getFileSystem().addResourcePath("Resources");
#endif

    eventHandler.keyboardHandler = std::bind(&SpineSample::handleKeyboard, this, std::placeholders::_1);
    eventHandler.mouseHandler = std::bind(&SpineSample::handleMouse, this, std::placeholders::_1);
    eventHandler.touchHandler = std::bind(&SpineSample::handleTouch, this, std::placeholders::_1);
    eventHandler.gamepadHandler = std::bind(&SpineSample::handleGamepad, this, std::placeholders::_1);
    eventHandler.uiHandler = std::bind(&SpineSample::handleUI, this, std::placeholders::_1);

    engine->getEventDispatcher().addEventHandler(&eventHandler);

    engine->getRenderer()->setClearColor(Color(64, 0, 0));

    engine->getSceneManager().setScene(&scene);

    cameraActor.addComponent(&camera);
    layer.addChild(&cameraActor);
    scene.addLayer(&layer);

    bundle.loadAsset(assets::Loader::Type::IMAGE, "spineboy.png");
    spineBoy.reset(new spine::SpineDrawable("spineboy.atlas", "spineboy.skel"));

    actor.addComponent(spineBoy.get());
    actor.setPosition({0, -100});
    actor.setScale(Vector2(0.4f, 0.4f));
    layer.addChild(&actor);

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

    engine->getInputManager()->startDeviceDiscovery();
}

bool SpineSample::handleKeyboard(const KeyboardEvent& event)
{
    if (event.type == ouzel::Event::Type::KEY_PRESS)
    {
        Vector3 position = cameraActor.getPosition();

        switch (event.key)
        {
            case input::Keyboard::Key::UP:
                position.y += 10.0f;
                break;
            case input::Keyboard::Key::DOWN:
                position.y -= 10.0f;
                break;
            case input::Keyboard::Key::LEFT:
                position.x -= 10.0f;
                break;
            case input::Keyboard::Key::RIGHT:
                position.x += 10.0f;
                break;
            case input::Keyboard::Key::ENTER:
                engine->getWindow()->setSize(Size2(640.0f, 480.0f));
                break;
            default:
                break;
        }

        cameraActor.setPosition(position);
    }

    return false;
}

bool SpineSample::handleMouse(const MouseEvent& event) const
{
    switch (event.type)
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

    return false;
}

bool SpineSample::handleTouch(const TouchEvent& event) const
{
    return false;
}

bool SpineSample::handleGamepad(const GamepadEvent& event) const
{
    if (event.type == ouzel::Event::Type::GAMEPAD_BUTTON_CHANGE)
    {

    }

    return false;
}

bool SpineSample::handleUI(const UIEvent& event) const
{
    return false;
}
