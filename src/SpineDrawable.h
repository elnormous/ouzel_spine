// Copyright (C) 2015 Elviss Strazdins

#pragma once

#include <ouzel.h>

#ifndef SPINE_MESH_VERTEX_COUNT_MAX
#define SPINE_MESH_VERTEX_COUNT_MAX 1000
#endif

struct spSkeletonData;
struct spSkeleton;
struct spAtlas;
struct spAnimationState;
struct spAnimationStateData;
struct spSkeletonBounds;
struct spEvent;
struct spTrackEntry;

namespace spine
{
    class SpineDrawable: public ouzel::scene::Component
    {
    public:
        struct Event
        {
            enum class Type
            {
                NONE,
                START,
                END,
                COMPLETE,
                EVENT
            };

            Type type = Event::Type::NONE;
            std::string name;
            float time = 0.0f;
            int32_t intValue = 0;
            float floatValue = 0.0f;
            std::string stringValue;
        };

        SpineDrawable(const std::string& atlasFile, const std::string& skeletonFile);
        virtual ~SpineDrawable();

        void update(float delta);
        virtual void draw(const ouzel::Matrix4& transformMatrix,
                          const ouzel::Color& color,
                          ouzel::scene::Camera* camera) override;

        virtual void drawWireframe(const ouzel::Matrix4& transformMatrix,
                                   const ouzel::Color& color,
                                   ouzel::scene::Camera* camera) override;

        float getTimeScale() const;
        void setTimeScale(float newTimeScale);

        void setOffset(const ouzel::Vector2& offset);
        ouzel::Vector2 getOffset();

        void setFlipX(bool flipX);
        bool getFlipX() const;

        void setFlipY(bool flipY);
        bool getFlipY() const;

        void reset();

        void clearTracks();
        void clearTrack(int32_t trackIndex);

        bool hasAnimation(const std::string& animationName);
        std::string getAnimation(int32_t trackIndex) const;
        bool setAnimation(int32_t trackIndex, const std::string& animationName, bool loop);
        bool addAnimation(int32_t trackIndex, const std::string& animationName, bool loop, float delay);

        bool setAnimationMix(const std::string& from, const std::string& to, float duration);
        bool setAnimationProgress(int32_t trackIndex, float progress);
        float getAnimationProgress(int32_t trackIndex) const;
        std::string getAnimationName(int32_t trackIndex) const;

        spSkeleton* getSkeleton() const { return skeleton; }
        spAtlas* getAtlas() const { return atlas; }
        spAnimationState* getAnimationState() const { return animationState; }

        void setEventCallback(const std::function<void(int32_t, const Event&)>& newEventCallback);
        void handleEvent(int type, spTrackEntry* entry, spEvent* event);

        void setSkin(const std::string& skinName);

    private:
        void updateBounds();

        spSkeletonData* skeletonData = nullptr;
        spSkeleton* skeleton = nullptr;
        spAtlas* atlas = nullptr;
        spAnimationState* animationState = nullptr;
        spAnimationStateData* animationStateData = nullptr;
        spSkeletonBounds* bounds = nullptr;

        std::vector<uint16_t> indices;
        std::vector<ouzel::graphics::VertexPCT> vertices;

        ouzel::graphics::MeshBufferPtr meshBuffer;
        ouzel::graphics::IndexBufferPtr indexBuffer;
        ouzel::graphics::VertexBufferPtr vertexBuffer;

        float worldVertices[SPINE_MESH_VERTEX_COUNT_MAX / sizeof(float)];
        
        ouzel::graphics::ShaderPtr shader;
        ouzel::graphics::BlendStatePtr blendState;
        ouzel::graphics::TexturePtr whitePixelTexture;

        ouzel::UpdateCallback updateCallback;

        std::function<void(int32_t, const Event&)> eventCallback;
    };
}
