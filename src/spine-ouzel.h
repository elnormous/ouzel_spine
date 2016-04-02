// Copyright (C) 2015 Elviss Strazdins

#pragma once

#include <ouzel.h>
#define SPINE_SHORT_NAMES
#include <spine/spine.h>
#include <spine/extension.h>

namespace spine
{
    class SkeletonDrawable: public ouzel::scene::Node
    {
    public:
        Skeleton* skeleton;
        AnimationState* state;
        float timeScale;
        ouzel::video::MeshBufferPtr meshBuffer;

        SkeletonDrawable (SkeletonData* skeleton, AnimationStateData* stateData = 0);
        ~SkeletonDrawable ();

        void update (float deltaTime);
        virtual void draw() const;
        
    private:
        bool ownsAnimationStateData;
        float* worldVertices;
    };
}
