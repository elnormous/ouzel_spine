// Copyright (C) 2017 Elviss Strazdins

#include "SpineDrawable.hpp"
#include "spine/spine.h"
#include "spine/extension.h"

static float worldVertices[SPINE_MESH_VERTEX_COUNT_MAX];

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

        if (skeletonFile.find(".json") != std::string::npos)
        {
            // is json format
            spSkeletonJson* json = spSkeletonJson_create(atlas);
            skeletonData = spSkeletonJson_readSkeletonDataFile(json, skeletonFile.c_str());
            
            if (!skeletonData)
            {
                ouzel::Log(ouzel::Log::Level::ERR) << "Failed to load skeleton: " << json->error;
                return;
            }
            spSkeletonJson_dispose(json);
        }
        else
        {
            // binary format
            spSkeletonBinary* binary = spSkeletonBinary_create(atlas);
            skeletonData = spSkeletonBinary_readSkeletonDataFile(binary, skeletonFile.c_str());
            
            if (!skeletonData)
            {
                ouzel::Log(ouzel::Log::Level::ERR) << "Failed to load skeleton: " << binary->error;
                return;
            }
            spSkeletonBinary_dispose(binary);
        }

        bounds = spSkeletonBounds_create();

        skeleton = spSkeleton_create(skeletonData);

        animationStateData = spAnimationStateData_create(skeletonData);
        
        animationState = spAnimationState_create(animationStateData);
        animationState->listener = listener;
        animationState->rendererObject = this;

        spSkeleton_setToSetupPose(skeleton);
        spSkeleton_updateWorldTransform(skeleton);

        updateBoundingBox();
        updateMaterials();

        indexBuffer = std::make_shared<ouzel::graphics::Buffer>();
        indexBuffer->init(ouzel::graphics::Buffer::Usage::INDEX, ouzel::graphics::Buffer::DYNAMIC);

        vertexBuffer = std::make_shared<ouzel::graphics::Buffer>();
        vertexBuffer->init(ouzel::graphics::Buffer::Usage::VERTEX, ouzel::graphics::Buffer::DYNAMIC);

        meshBuffer = std::make_shared<ouzel::graphics::MeshBuffer>();
        meshBuffer->init(sizeof(uint16_t), indexBuffer, ouzel::graphics::VertexPCT::ATTRIBUTES, vertexBuffer);

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
                             float opacity,
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
                        opacity,
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

        std::vector<std::vector<float>> pixelShaderConstants(1);
        std::vector<std::vector<float>> vertexShaderConstants(1);

        ouzel::Matrix4 modelViewProj = renderViewProjection * transformMatrix;
        vertexShaderConstants[0] = {std::begin(modelViewProj.m), std::end(modelViewProj.m)};

        ouzel::graphics::VertexPCT vertex;

        uint16_t currentVertexIndex = 0;
        indices.clear();
        vertices.clear();

        uint32_t offset = 0;

        boundingBox.reset();

        for (int i = 0; i < skeleton->slotsCount; ++i)
        {
            spSlot* slot = skeleton->drawOrder[i];
            spAttachment* attachment = slot->attachment;
            if (!attachment) continue;

            const std::shared_ptr<ouzel::graphics::Material>& material = materials[static_cast<size_t>(i)];

            float colorVector[] = {slot->color.r * material->diffuseColor.normR(),
                slot->color.g * material->diffuseColor.normG(),
                slot->color.b * material->diffuseColor.normB(),
                slot->color.a * material->diffuseColor.normA() * opacity};
            pixelShaderConstants[0] = {std::begin(colorVector), std::end(colorVector)};

            if (attachment->type == SP_ATTACHMENT_REGION)
            {
                spRegionAttachment* regionAttachment = reinterpret_cast<spRegionAttachment*>(attachment);

                spRegionAttachment_computeWorldVertices(regionAttachment, slot->bone, worldVertices, 0, 2);

                uint8_t r = static_cast<uint8_t>(skeleton->color.r * 255.0f);
                uint8_t g = static_cast<uint8_t>(skeleton->color.g * 255.0f);
                uint8_t b = static_cast<uint8_t>(skeleton->color.b * 255.0f);
                uint8_t a = static_cast<uint8_t>(skeleton->color.a * 255.0f);

                vertex.color.r = r;
                vertex.color.g = g;
                vertex.color.b = b;
                vertex.color.a = a;
                vertex.position.x = worldVertices[0];
                vertex.position.y = worldVertices[1];
                vertex.texCoord.x = regionAttachment->uvs[0];
                vertex.texCoord.y = regionAttachment->uvs[1];
                vertices.push_back(vertex);

                vertex.color.r = r;
                vertex.color.g = g;
                vertex.color.b = b;
                vertex.color.a = a;
                vertex.position.x = worldVertices[2];
                vertex.position.y = worldVertices[3];
                vertex.texCoord.x = regionAttachment->uvs[2];
                vertex.texCoord.y = regionAttachment->uvs[3];
                vertices.push_back(vertex);

                vertex.color.r = r;
                vertex.color.g = g;
                vertex.color.b = b;
                vertex.color.a = a;
                vertex.position.x = worldVertices[4];
                vertex.position.y = worldVertices[5];
                vertex.texCoord.x = regionAttachment->uvs[4];
                vertex.texCoord.y = regionAttachment->uvs[5];
                vertices.push_back(vertex);

                vertex.color.r = r;
                vertex.color.g = g;
                vertex.color.b = b;
                vertex.color.a = a;
                vertex.position.x = worldVertices[6];
                vertex.position.y = worldVertices[7];
                vertex.texCoord.x = regionAttachment->uvs[6];
                vertex.texCoord.y = regionAttachment->uvs[7];
                vertices.push_back(vertex);

                indices.push_back(currentVertexIndex + 0);
                indices.push_back(currentVertexIndex + 1);
                indices.push_back(currentVertexIndex + 2);
                indices.push_back(currentVertexIndex + 0);
                indices.push_back(currentVertexIndex + 2);
                indices.push_back(currentVertexIndex + 3);

                currentVertexIndex += 4;

                boundingBox.insertPoint(ouzel::Vector2(worldVertices[0], worldVertices[1]));
                boundingBox.insertPoint(ouzel::Vector2(worldVertices[2], worldVertices[3]));
                boundingBox.insertPoint(ouzel::Vector2(worldVertices[4], worldVertices[5]));
                boundingBox.insertPoint(ouzel::Vector2(worldVertices[6], worldVertices[7]));
            }
            else if (attachment->type == SP_ATTACHMENT_MESH)
            {
                spMeshAttachment* mesh = reinterpret_cast<spMeshAttachment*>(attachment);
                if (mesh->trianglesCount * 3 > SPINE_MESH_VERTEX_COUNT_MAX) continue;
                spVertexAttachment_computeWorldVertices(SUPER(mesh), slot, 0, mesh->super.worldVerticesLength, worldVertices, 0, 2);

                vertex.color.r = static_cast<uint8_t>(skeleton->color.r * 255.0f);
                vertex.color.g = static_cast<uint8_t>(skeleton->color.g * 255.0f);
                vertex.color.b = static_cast<uint8_t>(skeleton->color.b * 255.0f);
                vertex.color.a = static_cast<uint8_t>(skeleton->color.a * 255.0f);

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

                    boundingBox.insertPoint(ouzel::Vector2(worldVertices[index], worldVertices[index + 1]));
                }
            }
            else
            {
                continue;
            }

            if (indices.size() - offset > 0)
            {
                std::vector<std::shared_ptr<ouzel::graphics::Texture>> textures;
                if (wireframe) textures.push_back(whitePixelTexture);
                else textures.assign(std::begin(material->textures), std::end(material->textures));

                ouzel::sharedEngine->getRenderer()->addDrawCommand(textures,
                                                                   material->shader,
                                                                   pixelShaderConstants,
                                                                   vertexShaderConstants,
                                                                   material->blendState,
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
                                                                   material->cullMode);
            }

            offset = static_cast<uint32_t>(indices.size());
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
        skeleton->x = offset.x;
        skeleton->y = offset.y;

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

        updateMaterials();
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
                spRegionAttachment_computeWorldVertices(regionAttachment, slot->bone, worldVertices, 0, 2);

                boundingBox.insertPoint(ouzel::Vector2(worldVertices[0], worldVertices[1]));
                boundingBox.insertPoint(ouzel::Vector2(worldVertices[2], worldVertices[3]));
                boundingBox.insertPoint(ouzel::Vector2(worldVertices[4], worldVertices[5]));
                boundingBox.insertPoint(ouzel::Vector2(worldVertices[6], worldVertices[7]));
            }
            else if (attachment->type == SP_ATTACHMENT_MESH)
            {
                spMeshAttachment* mesh = reinterpret_cast<spMeshAttachment*>(attachment);
                if (mesh->trianglesCount * 3 > SPINE_MESH_VERTEX_COUNT_MAX) continue;
                spVertexAttachment_computeWorldVertices(SUPER(mesh), slot, 0, mesh->super.worldVerticesLength, worldVertices, 0, 2);

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

    void SpineDrawable::updateMaterials()
    {
        materials.clear();
        materials.resize(static_cast<size_t>(skeleton->slotsCount));

        for (int i = 0; i < skeleton->slotsCount; ++i)
        {
            std::shared_ptr<ouzel::graphics::Material>& material = materials[static_cast<size_t>(i)];
            material = std::make_shared<ouzel::graphics::Material>();
            material->shader = ouzel::sharedEngine->getCache()->getShader(ouzel::graphics::SHADER_TEXTURE);
            material->cullMode = ouzel::graphics::Renderer::CullMode::NONE;

            spSlot* slot = skeleton->drawOrder[i];
            spAttachment* attachment = slot->attachment;
            if (!attachment) continue;

            switch (slot->data->blendMode)
            {
                case SP_BLEND_MODE_ADDITIVE:
                    material->blendState = ouzel::sharedEngine->getCache()->getBlendState(ouzel::graphics::BLEND_ADD);
                    break;
                case SP_BLEND_MODE_MULTIPLY:
                    material->blendState = ouzel::sharedEngine->getCache()->getBlendState(ouzel::graphics::BLEND_MULTIPLY);
                    break;
                case SP_BLEND_MODE_SCREEN:
                    material->blendState = ouzel::sharedEngine->getCache()->getBlendState(ouzel::graphics::BLEND_SCREEN);
                    break;
                case SP_BLEND_MODE_NORMAL:
                default:
                    material->blendState = ouzel::sharedEngine->getCache()->getBlendState(ouzel::graphics::BLEND_ALPHA);
            }

            SpineTexture* texture = nullptr;

            if (attachment->type == SP_ATTACHMENT_REGION)
            {
                spRegionAttachment* regionAttachment = reinterpret_cast<spRegionAttachment*>(attachment);
                texture = static_cast<SpineTexture*>((static_cast<spAtlasRegion*>(regionAttachment->rendererObject))->page->rendererObject);
                if (texture) material->textures[0] = texture->texture;
            }
            else if (attachment->type == SP_ATTACHMENT_MESH)
            {
                spMeshAttachment* mesh = reinterpret_cast<spMeshAttachment*>(attachment);
                texture = static_cast<SpineTexture*>((static_cast<spAtlasRegion*>(mesh->rendererObject))->page->rendererObject);
                if (texture) material->textures[0] = texture->texture;
            }
        }

    }
}
