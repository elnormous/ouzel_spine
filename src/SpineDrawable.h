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
    class SpineDrawable: public ouzel::scene::Drawable
    {
    public:
        SpineDrawable(const std::string& atlasFile, const std::string& skeletonFile);
        virtual ~SpineDrawable();

        void update(float delta);
        virtual void draw(const ouzel::Matrix4& projection,
                          const ouzel::Matrix4& transform,
                          const ouzel::graphics::Color& color,
                          const ouzel::graphics::RenderTargetPtr& renderTarget,
                          const ouzel::scene::NodePtr& currentNode) override;

        float getTimeScale() const { return timeScale; }
        void setTimeScale(float newTimeScale) { timeScale = newTimeScale; }

        void setOffset(const ouzel::Vector2& offset);
        ouzel::Vector2 getOffset();

        void setFlipX(bool flipX);
        bool getFlipX() const;

        void setFlipY(bool flipY);
        bool getFlipY() const;

        void reset();

        void clearTracks();
        void clearTrack(int trackIndex);

        bool hasAnimation(const std::string& animationName);
        std::string getAnimation(int trackIndex) const;
        bool setAnimation(int trackIndex, const std::string& animationName, bool loop);
        bool addAnimation(int trackIndex, const std::string& animationName, bool loop, float delay);

        bool setAnimationMix(const std::string& from, const std::string& to, float duration);

        spSkeleton* getSkeleton() const { return skeleton; }
        spAtlas* getAtlas() const { return atlas; }
        spAnimationState* getAnimationState() const { return animationState; }

        void setEventCallback(const std::function<void(int, spEventType, spEvent*, int)>& newEventCallback);
        void handleEvent(int trackIndex, spEventType type, spEvent* event, int loopCount);
        
    private:
        spSkeleton* skeleton = nullptr;
        spAtlas* atlas = nullptr;
        spAnimationState* animationState = nullptr;
        spAnimationStateData* animationStateData = nullptr;
        spSkeletonBounds* bounds = nullptr;

        float timeScale = 1.0f;
        ouzel::graphics::MeshBufferPtr meshBuffer;

        float worldVertices[SPINE_MESH_VERTEX_COUNT_MAX / sizeof(float)];
        
        ouzel::graphics::ShaderPtr shader;
        ouzel::graphics::BlendStatePtr blendState;

        ouzel::UpdateCallback updateCallback;

        std::function<void(int, spEventType, spEvent*, int)> eventCallback;
    };
}
