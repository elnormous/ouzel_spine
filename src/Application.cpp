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

void callback (spAnimationState* state, int trackIndex, spEventType type, spEvent* event, int loopCount) {
    spTrackEntry* entry = spAnimationState_getCurrent(state, trackIndex);
    const char* animationName = (entry && entry->animation) ? entry->animation->name : 0;

    switch (type) {
        case SP_ANIMATION_START:
            printf("%d start: %s\n", trackIndex, animationName);
            break;
        case SP_ANIMATION_END:
            printf("%d end: %s\n", trackIndex, animationName);
            break;
        case SP_ANIMATION_COMPLETE:
            printf("%d complete: %s, %d\n", trackIndex, animationName, loopCount);
            break;
        case SP_ANIMATION_EVENT:
            printf("%d event: %s, %s: %d, %f, %s\n", trackIndex, animationName, event->data->name, event->intValue, event->floatValue,
                   event->stringValue);
            break;
    }
    fflush(stdout);
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

    spAtlas* atlas = spAtlas_createFromFile("witch1.atlas", 0);
    spSkeletonJson* json = spSkeletonJson_create(atlas);
    //json->scale = 0.6f;
    spSkeletonData *skeletonData = spSkeletonJson_readSkeletonDataFile(json, "witch1.json");
    if (!skeletonData) {
        printf("%s\n", json->error);
        exit(0);
    }
    spSkeletonJson_dispose(json);
    spSkeletonBounds* bounds = spSkeletonBounds_create();

    // Configure mixing.
    spAnimationStateData* stateData = spAnimationStateData_create(skeletonData);
    spAnimationStateData_setMixByName(stateData, "walk", "death", 0.5f);

    std::shared_ptr<spine::SkeletonDrawable> drawable = std::make_shared<spine::SkeletonDrawable>(skeletonData, stateData);
    drawable->timeScale = 1;

    drawable->state->listener = callback;

    spSkeleton* skeleton = drawable->skeleton;
    skeleton->flipX = false;
    skeleton->flipY = false;
    spSkeleton_setToSetupPose(skeleton);

    //skeleton->x = 320;
    skeleton->y = -200;
    spSkeleton_updateWorldTransform(skeleton);

    spSkeletonBounds_update(bounds, skeleton, true);

    spAnimationState_setAnimationByName(drawable->state, 0, "walk", true);
    spAnimationState_addAnimationByName(drawable->state, 0, "death", true, 5);
    
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
