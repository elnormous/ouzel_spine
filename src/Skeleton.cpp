// Copyright (C) 2015 Elviss Strazdins

#include <fstream>
#include "Skeleton.h"

struct SpineTexture
{
    std::shared_ptr<ouzel::graphics::Texture> texture;
};

void _spAtlasPage_createTexture(spAtlasPage* self, const char* path)
{
    SpineTexture* texture = new SpineTexture();

    texture->texture = ouzel::sharedEngine->getCache()->getTexture(path);
    self->rendererObject = texture;
    self->width = static_cast<int>(texture->texture->getSize().width);
    self->height = static_cast<int>(texture->texture->getSize().height);
}

void _spAtlasPage_disposeTexture(spAtlasPage* self)
{
    delete static_cast<SpineTexture*>(self->rendererObject);
}

char* _spUtil_readFile(const char* path, int* length)
{
    return _readFile(ouzel::sharedEngine->getFileSystem()->getPath(path).c_str(), length);
}

static void listener(spAnimationState* state, int trackIndex, spEventType type, spEvent* event, int loopCount)
{
    static_cast<spine::Skeleton*>(state->rendererObject)->handleEvent(trackIndex, type, event, loopCount);
}

namespace spine
{
    Skeleton::Skeleton(const std::string& atlasFile, const std::string& skeletonFile)
    {
        atlas = spAtlas_createFromFile(atlasFile.c_str(), 0);
        if (!atlas)
        {
            ouzel::log("Failed to load atlas");
            return;
        }

        spSkeletonJson* json = spSkeletonJson_create(atlas);
        //json->scale = 0.6f;
        spSkeletonData* skeletonData = spSkeletonJson_readSkeletonDataFile(json, skeletonFile.c_str());
        if (!skeletonData)
        {
            ouzel::log("Failed to load skeleton: %s", json->error);
            return;
        }
        spSkeletonJson_dispose(json);

        bounds = spSkeletonBounds_create();

        skeleton = spSkeleton_create(skeletonData);

        animationStateData = spAnimationStateData_create(skeletonData);
        
        animationState = spAnimationState_create(animationStateData);
        animationState->listener = listener;
        animationState->rendererObject = this;

        skeleton->flipX = false;
        skeleton->flipY = false;
        spSkeleton_setToSetupPose(skeleton);

        //skeleton->x = 320;
        //skeleton->y = -200;
        //spSkeleton_updateWorldTransform(_skeleton);

        meshBuffer = ouzel::sharedEngine->getRenderer()->createMeshBuffer();
        meshBuffer->setIndexSize(sizeof(uint16_t));
        meshBuffer->setVertexAttributes(ouzel::graphics::VertexPCT::ATTRIBUTES);

        blendState = ouzel::sharedEngine->getCache()->getBlendState(ouzel::graphics::BLEND_ALPHA);
        shader = ouzel::sharedEngine->getCache()->getShader(ouzel::graphics::SHADER_TEXTURE);

        updateCallback = std::make_shared<ouzel::UpdateCallback>();
        updateCallback->callback = std::bind(&Skeleton::update, this, std::placeholders::_1);
        ouzel::sharedEngine->scheduleUpdate(updateCallback);
    }

    Skeleton::~Skeleton()
    {
        if (bounds)
        {
            spSkeletonBounds_dispose(bounds);
        }

        if (atlas)
        {
            spAtlas_dispose(atlas);
        }

        if (animationState)
        {
            spAnimationState_dispose(animationState);
        }

        if (animationStateData)
        {
            spAnimationStateData_dispose(animationStateData);
        }

        if (skeleton) spSkeleton_dispose(skeleton);

        ouzel::sharedEngine->unscheduleUpdate(updateCallback);
    }

    void Skeleton::update(float delta)
    {
        spSkeleton_update(skeleton, delta);
        spAnimationState_update(animationState, delta * timeScale);
        spAnimationState_apply(animationState, skeleton);
        spSkeleton_updateWorldTransform(skeleton);

        spSkeletonBounds_update(bounds, skeleton, true);

        boundingBox.set(ouzel::Vector2(bounds->minX, bounds->minY),
                        ouzel::Vector2(bounds->maxX, bounds->maxY));
    }

    void Skeleton::draw(const ouzel::Matrix4& projection, const ouzel::Matrix4& transform, const ouzel::graphics::Color& color)
    {
        Drawable::draw(projection, transform, color);

        ouzel::sharedEngine->getRenderer()->activateBlendState(blendState);
        ouzel::sharedEngine->getRenderer()->activateShader(shader);

        ouzel::Matrix4 modelViewProj = projection * transform;
        float colorVector[] = { color.getR(), color.getG(), color.getB(), color.getA() };

        shader->setVertexShaderConstant(0, sizeof(ouzel::Matrix4), 1, modelViewProj.m);
        shader->setPixelShaderConstant(0, sizeof(colorVector), 1, colorVector);

        ouzel::graphics::VertexPCT vertex;

        uint16_t currentVertexIndex = 0;
        std::vector<uint16_t> indices;
        std::vector<ouzel::graphics::VertexPCT> vertices;

        ouzel::graphics::BlendStatePtr currentBlendState = blendState;
        uint32_t offset = 0;

        for (int i = 0; i < skeleton->slotsCount; ++i)
        {
            spSlot* slot = skeleton->drawOrder[i];
            spAttachment* attachment = slot->attachment;
            if (!attachment) continue;

            switch (slot->data->blendMode)
            {
                case SP_BLEND_MODE_ADDITIVE:
                    blendState = ouzel::sharedEngine->getCache()->getBlendState(ouzel::graphics::BLEND_ADD);
                    break;
                case SP_BLEND_MODE_MULTIPLY:
                    blendState = ouzel::sharedEngine->getCache()->getBlendState(ouzel::graphics::BLEND_MULTIPLY);
                    break;
                case SP_BLEND_MODE_SCREEN: // Unsupported, fall through.
                default:
                    blendState = ouzel::sharedEngine->getCache()->getBlendState(ouzel::graphics::BLEND_ALPHA);
            }
            if (currentBlendState != blendState)
            {
                if (indices.size() - offset > 0)
                {
                    meshBuffer->uploadIndices(indices.data(), static_cast<uint32_t>(indices.size()));
                    meshBuffer->uploadVertices(vertices.data(), static_cast<uint32_t>(vertices.size()));
                    ouzel::sharedEngine->getRenderer()->drawMeshBuffer(meshBuffer, offset);
                }

                currentBlendState = blendState;
                offset = static_cast<uint32_t>(indices.size());

                ouzel::sharedEngine->getRenderer()->activateBlendState(blendState);
            }

            SpineTexture* texture = 0;
            if (attachment->type == SP_ATTACHMENT_REGION)
            {
                spRegionAttachment* regionAttachment = reinterpret_cast<spRegionAttachment*>(attachment);
                texture = static_cast<SpineTexture*>((static_cast<spAtlasRegion*>(regionAttachment->rendererObject))->page->rendererObject);
                spRegionAttachment_computeWorldVertices(regionAttachment, slot->bone, worldVertices);

                uint8_t r = static_cast<uint8_t>(skeleton->r * slot->r * 255);
                uint8_t g = static_cast<uint8_t>(skeleton->g * slot->g * 255);
                uint8_t b = static_cast<uint8_t>(skeleton->b * slot->b * 255);
                uint8_t a = static_cast<uint8_t>(skeleton->a * slot->a * 255);

                vertex.color.r = r;
                vertex.color.g = g;
                vertex.color.b = b;
                vertex.color.a = a;
                vertex.position.x = worldVertices[SP_VERTEX_X1];
                vertex.position.y = worldVertices[SP_VERTEX_Y1];
                vertex.texCoord.x = regionAttachment->uvs[SP_VERTEX_X1];
                vertex.texCoord.y = regionAttachment->uvs[SP_VERTEX_Y1];
                vertices.push_back(vertex);

                vertex.color.r = r;
                vertex.color.g = g;
                vertex.color.b = b;
                vertex.color.a = a;
                vertex.position.x = worldVertices[SP_VERTEX_X2];
                vertex.position.y = worldVertices[SP_VERTEX_Y2];
                vertex.texCoord.x = regionAttachment->uvs[SP_VERTEX_X2];
                vertex.texCoord.y = regionAttachment->uvs[SP_VERTEX_Y2];
                vertices.push_back(vertex);

                vertex.color.r = r;
                vertex.color.g = g;
                vertex.color.b = b;
                vertex.color.a = a;
                vertex.position.x = worldVertices[SP_VERTEX_X3];
                vertex.position.y = worldVertices[SP_VERTEX_Y3];
                vertex.texCoord.x = regionAttachment->uvs[SP_VERTEX_X3];
                vertex.texCoord.y = regionAttachment->uvs[SP_VERTEX_Y3];
                vertices.push_back(vertex);

                vertex.color.r = r;
                vertex.color.g = g;
                vertex.color.b = b;
                vertex.color.a = a;
                vertex.position.x = worldVertices[SP_VERTEX_X4];
                vertex.position.y = worldVertices[SP_VERTEX_Y4];
                vertex.texCoord.x = regionAttachment->uvs[SP_VERTEX_X4];
                vertex.texCoord.y = regionAttachment->uvs[SP_VERTEX_Y4];
                vertices.push_back(vertex);

                indices.push_back(currentVertexIndex + 0);
                indices.push_back(currentVertexIndex + 1);
                indices.push_back(currentVertexIndex + 2);
                indices.push_back(currentVertexIndex + 0);
                indices.push_back(currentVertexIndex + 2);
                indices.push_back(currentVertexIndex + 3);

                currentVertexIndex += 4;
            }
            else if (attachment->type == SP_ATTACHMENT_MESH)
            {
                spMeshAttachment* mesh = reinterpret_cast<spMeshAttachment*>(attachment);
                if (mesh->verticesCount > SPINE_MESH_VERTEX_COUNT_MAX) continue;
                texture = static_cast<SpineTexture*>((static_cast<spAtlasRegion*>(mesh->rendererObject))->page->rendererObject);
                spMeshAttachment_computeWorldVertices(mesh, slot, worldVertices);

                uint8_t r = static_cast<uint8_t>(skeleton->r * slot->r * 255);
                uint8_t g = static_cast<uint8_t>(skeleton->g * slot->g * 255);
                uint8_t b = static_cast<uint8_t>(skeleton->b * slot->b * 255);
                uint8_t a = static_cast<uint8_t>(skeleton->a * slot->a * 255);
                vertex.color.r = r;
                vertex.color.g = g;
                vertex.color.b = b;
                vertex.color.a = a;

                for (int t = 0; t < mesh->trianglesCount; ++t)
                {
                    int index = mesh->triangles[t] << 1;
                    vertex.position.x = worldVertices[index];
                    vertex.position.y = worldVertices[index + 1];
                    vertex.texCoord.x = mesh->uvs[index];
                    vertex.texCoord.y = mesh->uvs[index + 1];

                    indices.push_back(currentVertexIndex);
                    currentVertexIndex++;
                    vertices.push_back(vertex);
                }
                
            }
            else if (attachment->type == SP_ATTACHMENT_WEIGHTED_MESH)
            {
                spWeightedMeshAttachment* mesh = reinterpret_cast<spWeightedMeshAttachment*>(attachment);
                if (mesh->uvsCount > SPINE_MESH_VERTEX_COUNT_MAX) continue;
                texture = static_cast<SpineTexture*>((static_cast<spAtlasRegion*>(mesh->rendererObject))->page->rendererObject);
                spWeightedMeshAttachment_computeWorldVertices(mesh, slot, worldVertices);
                
                uint8_t r = static_cast<uint8_t>(skeleton->r * slot->r * 255);
                uint8_t g = static_cast<uint8_t>(skeleton->g * slot->g * 255);
                uint8_t b = static_cast<uint8_t>(skeleton->b * slot->b * 255);
                uint8_t a = static_cast<uint8_t>(skeleton->a * slot->a * 255);
                vertex.color.r = r;
                vertex.color.g = g;
                vertex.color.b = b;
                vertex.color.a = a;

                for (int t = 0; t < mesh->trianglesCount; ++t)
                {
                    int index = mesh->triangles[t] << 1;
                    vertex.position.x = worldVertices[index];
                    vertex.position.y = worldVertices[index + 1];
                    vertex.texCoord.x = mesh->uvs[index];
                    vertex.texCoord.y = mesh->uvs[index + 1];

                    indices.push_back(currentVertexIndex);
                    currentVertexIndex++;
                    vertices.push_back(vertex);
                }
            }

            if (texture && texture->texture)
            {
                ouzel::sharedEngine->getRenderer()->activateTexture(texture->texture, 0);
            }
        }

        if (indices.size() - offset > 0)
        {
            meshBuffer->uploadIndices(indices.data(), static_cast<uint32_t>(indices.size()));
            meshBuffer->uploadVertices(vertices.data(), static_cast<uint32_t>(vertices.size()));
            ouzel::sharedEngine->getRenderer()->drawMeshBuffer(meshBuffer, offset);
        }
    }

    void Skeleton::setAnimation(int trackIndex, const std::string& animationName, bool loop)
    {
        spAnimationState_setAnimationByName(animationState, trackIndex, animationName.c_str(), loop ? 1 : 0);
    }

    void Skeleton::addAnimation(int trackIndex, const std::string& animationName, bool loop, float delay)
    {
        spAnimationState_addAnimationByName(animationState, trackIndex, animationName.c_str(), loop ? 1 : 0, delay);
    }

    void Skeleton::setAnimationMix(const std::string& from, const std::string& to, float duration)
    {
        // Configure mixing
        spAnimationStateData_setMixByName(animationStateData, from.c_str(), to.c_str(), duration);
    }

    void Skeleton::setEventCallback(const std::function<void(int, spEventType, spEvent*, int)>& newEventCallback)
    {
        eventCallback = newEventCallback;
    }

    void Skeleton::handleEvent(int trackIndex, spEventType type, spEvent* event, int loopCount)
    {
        if (eventCallback)
        {
            eventCallback(trackIndex, type, event, loopCount);
        }
    }
}
