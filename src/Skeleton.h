// Copyright (C) 2015 Elviss Strazdins

#pragma once

#include <ouzel.h>
#include <spine/spine.h>
#include <spine/extension.h>

#ifndef SPINE_MESH_VERTEX_COUNT_MAX
#define SPINE_MESH_VERTEX_COUNT_MAX 1000
#endif

namespace spine
{
    class Skeleton: public ouzel::scene::Drawable
    {
    public:
        Skeleton(const std::string& atlasFile, const std::string& skeletonFile);
        ~Skeleton();

        void update(float delta);
        virtual void draw(const ouzel::Matrix4& projection, const ouzel::Matrix4& transform, const ouzel::graphics::Color& color) override;

        float getTimeScale() const { return _timeScale; }
        void setTimeScale(float timeScale) { _timeScale = timeScale; }

        void setAnimation(int trackIndex, const std::string& animationName, bool loop);
        void addAnimation(int trackIndex, const std::string& animationName, bool loop, float delay);

        void setAnimationMix(const std::string& from, const std::string& to, float duration);

        spSkeleton* getSkeleton() const { return _skeleton; }
        spAtlas* getAtlas() const { return _atlas; }
        spAnimationState* getAnimationState() const { return _animationState; }

        void setEventCallback(const std::function<void(int, spEventType, spEvent*, int)>& eventCallback);
        void handleEvent(int trackIndex, spEventType type, spEvent* event, int loopCount);
        
    private:
        spSkeleton* _skeleton = nullptr;
        spAtlas* _atlas = nullptr;
        spAnimationState* _animationState = nullptr;
        spAnimationStateData* _animationStateData = nullptr;
        spSkeletonBounds* _bounds = nullptr;

        float _timeScale = 1.0f;
        ouzel::graphics::MeshBufferPtr _meshBuffer;

        float _worldVertices[SPINE_MESH_VERTEX_COUNT_MAX / sizeof(float)];
        
        ouzel::graphics::ShaderPtr _shader;
        ouzel::graphics::BlendStatePtr _blendState;

        ouzel::UpdateCallbackPtr _updateCallback;

        std::function<void(int, spEventType, spEvent*, int)> _eventCallback;
    };
}
