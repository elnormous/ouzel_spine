// Copyright (C) 2015 Elviss Strazdins

#pragma once

#include <ouzel.h>
#include <spine/spine.h>
#include <spine/extension.h>

namespace spine
{
    class SkeletonDrawable: public ouzel::scene::Node
    {
    public:
        spSkeleton* skeleton;
        spAnimationState* state;
        float timeScale;
        ouzel::video::MeshBufferPtr meshBuffer;

        SkeletonDrawable (spSkeletonData* skeleton, spAnimationStateData* stateData = 0);
        ~SkeletonDrawable ();

        void update(float delta);
        virtual void draw() override;
        
    private:
        bool ownsAnimationStateData;
        float* worldVertices;
        
        ouzel::video::ShaderPtr _shader;
        ouzel::video::BlendStatePtr _blendState;

        uint32_t _uniModelViewProj;

        ouzel::UpdateCallbackPtr _updateCallback;
    };
}
