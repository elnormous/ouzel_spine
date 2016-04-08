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
    class SkeletonDrawable: public ouzel::scene::Node
    {
    public:
        SkeletonDrawable(const std::string& atlasFile, const std::string& skeletonFile);
        ~SkeletonDrawable();

        void update(float delta);
        virtual void draw() override;
        
    private:
        spSkeleton* _skeleton = nullptr;
        spAnimationState* _state = nullptr;
        float _timeScale = 1.0f;
        ouzel::video::MeshBufferPtr _meshBuffer;

        float _worldVertices[SPINE_MESH_VERTEX_COUNT_MAX / sizeof(float)];
        
        ouzel::video::ShaderPtr _shader;
        ouzel::video::BlendStatePtr _blendState;

        uint32_t _uniModelViewProj;

        ouzel::UpdateCallbackPtr _updateCallback;
    };
}
