// Copyright (C) 2017 Elviss Strazdins

#include "spine/spine.h"
#include "spine/extension.h"
#include "SpineDrawable.hpp"

struct SpineTexture
{
    std::shared_ptr<ouzel::graphics::Texture> texture;
};

void _spAtlasPage_createTexture(spAtlasPage* self, const char* path)
{
    SpineTexture* texture = new SpineTexture();

    texture->texture = ouzel::sharedEngine->getCache()->getTexture(path);
    self->rendererObject = texture;
    self->width = static_cast<int>(texture->texture->getSize().v[0]);
    self->height = static_cast<int>(texture->texture->getSize().v[1]);
}

void _spAtlasPage_disposeTexture(spAtlasPage* self)
{
    delete static_cast<SpineTexture*>(self->rendererObject);
}

char* _spUtil_readFile(const char* path, int* length)
{
    char* result;
    std::vector<uint8_t> data;

    ouzel::sharedEngine->getFileSystem()->readFile(path, data);
    *length = static_cast<int>(data.size());

    result = MALLOC(char, data.size());
    memcpy(result, data.data(), data.size());

    return result;
}

static void listener(spAnimationState* state, spEventType type, spTrackEntry* entry, spEvent* event)
{
    static_cast<spine::SpineDrawable*>(state->rendererObject)->handleEvent(type, entry, event);
}

namespace spine
{
    SpineDrawable::SpineDrawable(const std::string& atlasFile, const std::string& skeletonFile):
        Component(TYPE)
    {
        atlas = spAtlas_createFromFile(atlasFile.c_str(), 0);
        if (!atlas)
        {
            ouzel::Log(ouzel::Log::Level::ERR) << "Failed to load atlas";
            return;
        }

        spSkeletonJson* json = spSkeletonJson_create(atlas);
        //json->scale = 0.6f;
        skeletonData = spSkeletonJson_readSkeletonDataFile(json, skeletonFile.c_str());
        if (!skeletonData)
        {
            ouzel::Log(ouzel::Log::Level::ERR) << "Failed to load skeleton: " << json->error;
            return;
        }
        spSkeletonJson_dispose(json);

        bounds = spSkeletonBounds_create();

        skeleton = spSkeleton_create(skeletonData);

        animationStateData = spAnimationStateData_create(skeletonData);
        
        animationState = spAnimationState_create(animationStateData);
        animationState->listener = listener;
        animationState->rendererObject = this;

        spSkeleton_setToSetupPose(skeleton);
        spSkeleton_updateWorldTransform(skeleton);

        updateBoundingBox();

        indexBuffer = std::make_shared<ouzel::graphics::Buffer>();
        indexBuffer->init(ouzel::graphics::Buffer::Usage::INDEX, ouzel::graphics::Buffer::DYNAMIC);

        vertexBuffer = std::make_shared<ouzel::graphics::Buffer>();
        vertexBuffer->init(ouzel::graphics::Buffer::Usage::VERTEX, ouzel::graphics::Buffer::DYNAMIC);

        meshBuffer = std::make_shared<ouzel::graphics::MeshBuffer>();
        meshBuffer->init(sizeof(uint16_t), indexBuffer, ouzel::graphics::VertexPCT::ATTRIBUTES, vertexBuffer);

        blendState = ouzel::sharedEngine->getCache()->getBlendState(ouzel::graphics::BLEND_ALPHA);
        shader = ouzel::sharedEngine->getCache()->getShader(ouzel::graphics::SHADER_TEXTURE);
        whitePixelTexture = ouzel::sharedEngine->getCache()->getTexture(ouzel::graphics::TEXTURE_WHITE_PIXEL);

        updateCallback.callback = std::bind(&SpineDrawable::update, this, std::placeholders::_1);
        ouzel::sharedEngine->scheduleUpdate(&updateCallback);
    }

    SpineDrawable::~SpineDrawable()
    {
        if (bounds) spSkeletonBounds_dispose(bounds);
        if (atlas) spAtlas_dispose(atlas);
        if (animationState) spAnimationState_dispose(animationState);
        if (animationStateData) spAnimationStateData_dispose(animationStateData);
        if (skeleton) spSkeleton_dispose(skeleton);
        if (skeletonData) spSkeletonData_dispose(skeletonData);
    }

    void SpineDrawable::update(float delta)
    {
        spSkeleton_update(skeleton, delta);
        spAnimationState_update(animationState, delta);
    }

    void SpineDrawable::draw(const ouzel::Matrix4& transformMatrix,
                             const ouzel::Color& drawColor,
                             const ouzel::Matrix4& renderViewProjection,
                             const std::shared_ptr<ouzel::graphics::Texture>& renderTarget,
                             const ouzel::Rectangle& renderViewport,
                             bool depthWrite,
                             bool depthTest,
                             bool wireframe,
                             bool scissorTest,
                             const ouzel::Rectangle& scissorRectangle)
    {
        Component::draw(transformMatrix,
                        drawColor,
                        renderViewProjection,
                        renderTarget,
                        renderViewport,
                        depthWrite,
                        depthTest,
                        wireframe,
                        scissorTest,
                        scissorRectangle);

        spAnimationState_apply(animationState, skeleton);
        spSkeleton_updateWorldTransform(skeleton);

        std::shared_ptr<ouzel::graphics::Texture> currentTexture;

        ouzel::Matrix4 modelViewProj = renderViewProjection * transformMatrix;
        float colorVector[] = {drawColor.normR(), drawColor.normG(), drawColor.normB(), drawColor.normA()};

        std::vector<std::vector<float>> pixelShaderConstants(1);
        pixelShaderConstants[0] = {std::begin(colorVector), std::end(colorVector)};

        std::vector<std::vector<float>> vertexShaderConstants(1);
        vertexShaderConstants[0] = {std::begin(modelViewProj.m), std::end(modelViewProj.m)};

        ouzel::graphics::VertexPCT vertex;

        uint16_t currentVertexIndex = 0;
        indices.clear();
        vertices.clear();

        std::shared_ptr<ouzel::graphics::BlendState> currentBlendState = blendState;
        uint32_t offset = 0;

        boundingBox.reset();

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
                case SP_BLEND_MODE_NORMAL:
                default:
                    blendState = ouzel::sharedEngine->getCache()->getBlendState(ouzel::graphics::BLEND_ALPHA);
            }
            if (currentBlendState != blendState)
            {
                if (indices.size() - offset > 0)
                {
                    ouzel::sharedEngine->getRenderer()->addDrawCommand({wireframe ? whitePixelTexture : currentTexture},
                                                                       shader,
                                                                       pixelShaderConstants,
                                                                       vertexShaderConstants,
                                                                       blendState,
                                                                       meshBuffer,
                                                                       static_cast<uint32_t>(indices.size()) - offset,
                                                                       ouzel::graphics::Renderer::DrawMode::TRIANGLE_LIST,
                                                                       offset,
                                                                       renderTarget,
                                                                       renderViewport,
                                                                       depthWrite,
                                                                       depthTest,
                                                                       wireframe,
                                                                       scissorTest,
                                                                       scissorRectangle,
                                                                       ouzel::graphics::Renderer::CullMode::NONE);
                }

                currentBlendState = blendState;
                offset = static_cast<uint32_t>(indices.size());
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

                vertex.color.v[0] = r;
                vertex.color.v[1] = g;
                vertex.color.v[2] = b;
                vertex.color.v[3] = a;
                vertex.position.v[0] = worldVertices[SP_VERTEX_X1];
                vertex.position.v[1] = worldVertices[SP_VERTEX_Y1];
                vertex.texCoord.v[0] = regionAttachment->uvs[SP_VERTEX_X1];
                vertex.texCoord.v[1] = regionAttachment->uvs[SP_VERTEX_Y1];
                vertices.push_back(vertex);

                vertex.color.v[0] = r;
                vertex.color.v[1] = g;
                vertex.color.v[2] = b;
                vertex.color.v[3] = a;
                vertex.position.v[0] = worldVertices[SP_VERTEX_X2];
                vertex.position.v[1] = worldVertices[SP_VERTEX_Y2];
                vertex.texCoord.v[0] = regionAttachment->uvs[SP_VERTEX_X2];
                vertex.texCoord.v[1] = regionAttachment->uvs[SP_VERTEX_Y2];
                vertices.push_back(vertex);

                vertex.color.v[0] = r;
                vertex.color.v[1] = g;
                vertex.color.v[2] = b;
                vertex.color.v[3] = a;
                vertex.position.v[0] = worldVertices[SP_VERTEX_X3];
                vertex.position.v[1] = worldVertices[SP_VERTEX_Y3];
                vertex.texCoord.v[0] = regionAttachment->uvs[SP_VERTEX_X3];
                vertex.texCoord.v[1] = regionAttachment->uvs[SP_VERTEX_Y3];
                vertices.push_back(vertex);

                vertex.color.v[0] = r;
                vertex.color.v[1] = g;
                vertex.color.v[2] = b;
                vertex.color.v[3] = a;
                vertex.position.v[0] = worldVertices[SP_VERTEX_X4];
                vertex.position.v[1] = worldVertices[SP_VERTEX_Y4];
                vertex.texCoord.v[0] = regionAttachment->uvs[SP_VERTEX_X4];
                vertex.texCoord.v[1] = regionAttachment->uvs[SP_VERTEX_Y4];
                vertices.push_back(vertex);

                indices.push_back(currentVertexIndex + 0);
                indices.push_back(currentVertexIndex + 1);
                indices.push_back(currentVertexIndex + 2);
                indices.push_back(currentVertexIndex + 0);
                indices.push_back(currentVertexIndex + 2);
                indices.push_back(currentVertexIndex + 3);

                currentVertexIndex += 4;

                boundingBox.insertPoint(ouzel::Vector2(worldVertices[SP_VERTEX_X1], worldVertices[SP_VERTEX_Y1]));
                boundingBox.insertPoint(ouzel::Vector2(worldVertices[SP_VERTEX_X2], worldVertices[SP_VERTEX_Y2]));
                boundingBox.insertPoint(ouzel::Vector2(worldVertices[SP_VERTEX_X3], worldVertices[SP_VERTEX_Y3]));
                boundingBox.insertPoint(ouzel::Vector2(worldVertices[SP_VERTEX_X4], worldVertices[SP_VERTEX_Y4]));
            }
            else if (attachment->type == SP_ATTACHMENT_MESH)
            {
                spMeshAttachment* mesh = reinterpret_cast<spMeshAttachment*>(attachment);
                if (mesh->trianglesCount * 3 > SPINE_MESH_VERTEX_COUNT_MAX) continue;
                texture = static_cast<SpineTexture*>((static_cast<spAtlasRegion*>(mesh->rendererObject))->page->rendererObject);
                spMeshAttachment_computeWorldVertices(mesh, slot, worldVertices);

                vertex.color.v[0] = static_cast<uint8_t>(skeleton->r * slot->r * 255);
                vertex.color.v[1] = static_cast<uint8_t>(skeleton->g * slot->g * 255);
                vertex.color.v[2] = static_cast<uint8_t>(skeleton->b * slot->b * 255);
                vertex.color.v[3] = static_cast<uint8_t>(skeleton->a * slot->a * 255);

                for (int t = 0; t < mesh->trianglesCount; ++t)
                {
                    int index = mesh->triangles[t] << 1;
                    vertex.position.v[0] = worldVertices[index];
                    vertex.position.v[1] = worldVertices[index + 1];
                    vertex.texCoord.v[0] = mesh->uvs[index];
                    vertex.texCoord.v[1] = mesh->uvs[index + 1];

                    indices.push_back(currentVertexIndex);
                    currentVertexIndex++;
                    vertices.push_back(vertex);

                    boundingBox.insertPoint(ouzel::Vector2(worldVertices[index], worldVertices[index + 1]));
                }
            }
            else
            {
                continue;
            }

            if (texture && texture->texture)
            {
                currentTexture = texture->texture;
            }
        }

        if (indices.size() - offset > 0)
        {
            ouzel::sharedEngine->getRenderer()->addDrawCommand({wireframe ? whitePixelTexture : currentTexture},
                                                               shader,
                                                               pixelShaderConstants,
                                                               vertexShaderConstants,
                                                               blendState,
                                                               meshBuffer,
                                                               static_cast<uint32_t>(indices.size()) - offset,
                                                               ouzel::graphics::Renderer::DrawMode::TRIANGLE_LIST,
                                                               offset,
                                                               renderTarget,
                                                               renderViewport,
                                                               depthWrite,
                                                               depthTest,
                                                               wireframe,
                                                               scissorTest,
                                                               scissorRectangle,
                                                               ouzel::graphics::Renderer::CullMode::NONE);
        }


        indexBuffer->setData(indices.data(), static_cast<uint32_t>(ouzel::getVectorSize(indices)));
        vertexBuffer->setData(vertices.data(), static_cast<uint32_t>(ouzel::getVectorSize(vertices)));
    }

    float SpineDrawable::getTimeScale() const
    {
        return animationState->timeScale;
    }

    void SpineDrawable::setTimeScale(float newTimeScale)
    {
        animationState->timeScale = newTimeScale;
    }

    void SpineDrawable::setFlipX(bool flipX)
    {
        skeleton->flipX = flipX;
    }

    bool SpineDrawable::getFlipX() const
    {
        return skeleton->flipX != 0;
    }

    void SpineDrawable::setFlipY(bool flipY)
    {
        skeleton->flipY = flipY;
    }

    bool SpineDrawable::getFlipY() const
    {
        return skeleton->flipY != 0;
    }

    void SpineDrawable::setOffset(const ouzel::Vector2& offset)
    {
        skeleton->x = offset.v[0];
        skeleton->y = offset.v[1];

        spSkeleton_updateWorldTransform(skeleton);
    }

    ouzel::Vector2 SpineDrawable::getOffset()
    {
        return ouzel::Vector2(skeleton->x, skeleton->y);
    }

    void SpineDrawable::reset()
    {
        spSkeleton_setToSetupPose(skeleton);
    }

    void SpineDrawable::clearTracks()
    {
        spAnimationState_clearTracks(animationState);
    }

    void SpineDrawable::clearTrack(int32_t trackIndex)
    {
        spAnimationState_clearTrack(animationState, trackIndex);
    }

    bool SpineDrawable::hasAnimation(const std::string& animationName)
    {
        spAnimation* animation = spSkeletonData_findAnimation(animationState->data->skeletonData, animationName.c_str());

        return animation != nullptr;
    }

    std::string SpineDrawable::getAnimation(int32_t trackIndex) const
    {
        spTrackEntry* track = spAnimationState_getCurrent(animationState, trackIndex);

        if (track && track->animation)
        {
            return track->animation->name;
        }

        return "";
    }

    bool SpineDrawable::setAnimation(int32_t trackIndex, const std::string& animationName, bool loop)
    {
        spAnimation* animation = spSkeletonData_findAnimation(animationState->data->skeletonData, animationName.c_str());

        if (!animation)
        {
            return false;
        }

        spAnimationState_setAnimation(animationState, trackIndex, animation, loop ? 1 : 0);

        return true;
    }

    bool SpineDrawable::addAnimation(int32_t trackIndex, const std::string& animationName, bool loop, float delay)
    {
        spAnimation* animation = spSkeletonData_findAnimation(animationState->data->skeletonData, animationName.c_str());

        if (!animation)
        {
            return false;
        }

        spAnimationState_addAnimation(animationState, trackIndex, animation, loop ? 1 : 0, delay);

        return true;
    }

    bool SpineDrawable::setAnimationMix(const std::string& from, const std::string& to, float duration)
    {
        spAnimation* animationFrom = spSkeletonData_findAnimation(animationState->data->skeletonData, from.c_str());

        if (!animationFrom)
        {
            return false;
        }

        spAnimation* animationTo = spSkeletonData_findAnimation(animationState->data->skeletonData, to.c_str());

        if (!animationTo)
        {
            return false;
        }

        spAnimationStateData_setMix(animationStateData, animationFrom, animationTo, duration);

        return true;
    }

    bool SpineDrawable::setAnimationProgress(int32_t trackIndex, float progress)
    {
        if (spTrackEntry* current = spAnimationState_getCurrent(animationState, trackIndex))
        {
            current->trackTime = current->trackEnd * progress;
        }

        return true;
    }

    float SpineDrawable::getAnimationProgress(int32_t trackIndex) const
    {
        if (spTrackEntry* current = spAnimationState_getCurrent(animationState, trackIndex))
        {
            return (current->trackEnd != 0.0f) ? current->trackTime / current->trackEnd : 0.0f;
        }

        return 0.0f;
    }

    std::string SpineDrawable::getAnimationName(int32_t trackIndex) const
    {
        if (spTrackEntry* current = spAnimationState_getCurrent(animationState, trackIndex))
        {
            if (current->animation) return current->animation->name;
        }

        return std::string();
    }

    void SpineDrawable::setEventCallback(const std::function<void(int32_t, const Event&)>& newEventCallback)
    {
        eventCallback = newEventCallback;
    }

    void SpineDrawable::handleEvent(int type, spTrackEntry* entry, spEvent* event)
    {
        if (eventCallback)
        {
            Event e;

            switch (type)
            {
                case SP_ANIMATION_START:
                    e.type = Event::Type::START;
                    break;
                case SP_ANIMATION_END:
                    e.type = Event::Type::END;
                    break;
                case SP_ANIMATION_COMPLETE:
                    e.type = Event::Type::COMPLETE;
                    break;
                case SP_ANIMATION_EVENT:
                    e.type = Event::Type::EVENT;
                    if (event)
                    {
                        e.name = event->data->name;
                        e.time = event->time;
                        e.intValue = event->intValue;
                        e.floatValue = event->floatValue;
                        if (event->stringValue) e.stringValue = event->stringValue;
                    }
                    break;
            }

            eventCallback(entry->trackIndex, e);
        }
    }

    void SpineDrawable::setSkin(const std::string& skinName)
    {
        spSkin* skin = spSkeletonData_findSkin(skeletonData, skinName.c_str());
        spSkeleton_setSkin(skeleton, skin);

        updateBoundingBox();
    }

    void SpineDrawable::updateBoundingBox()
    {
        boundingBox.reset();

        for (int i = 0; i < skeleton->slotsCount; ++i)
        {
            spSlot* slot = skeleton->drawOrder[i];
            spAttachment* attachment = slot->attachment;
            if (!attachment) continue;

            if (attachment->type == SP_ATTACHMENT_REGION)
            {
                spRegionAttachment* regionAttachment = reinterpret_cast<spRegionAttachment*>(attachment);
                spRegionAttachment_computeWorldVertices(regionAttachment, slot->bone, worldVertices);

                boundingBox.insertPoint(ouzel::Vector2(worldVertices[SP_VERTEX_X1], worldVertices[SP_VERTEX_Y1]));
                boundingBox.insertPoint(ouzel::Vector2(worldVertices[SP_VERTEX_X2], worldVertices[SP_VERTEX_Y2]));
                boundingBox.insertPoint(ouzel::Vector2(worldVertices[SP_VERTEX_X3], worldVertices[SP_VERTEX_Y3]));
                boundingBox.insertPoint(ouzel::Vector2(worldVertices[SP_VERTEX_X4], worldVertices[SP_VERTEX_Y4]));
            }
            else if (attachment->type == SP_ATTACHMENT_MESH)
            {
                spMeshAttachment* mesh = reinterpret_cast<spMeshAttachment*>(attachment);
                if (mesh->trianglesCount * 3 > SPINE_MESH_VERTEX_COUNT_MAX) continue;
                spMeshAttachment_computeWorldVertices(mesh, slot, worldVertices);

                for (int t = 0; t < mesh->trianglesCount; ++t)
                {
                    int index = mesh->triangles[t] << 1;

                    boundingBox.insertPoint(ouzel::Vector2(worldVertices[index], worldVertices[index + 1]));
                }
            }
            else
            {
                continue;
            }
        }
    }
}
