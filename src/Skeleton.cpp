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
        _atlas = spAtlas_createFromFile(atlasFile.c_str(), 0);
        if (!_atlas)
        {
            ouzel::log("Failed to load atlas");
            return;
        }

        spSkeletonJson* json = spSkeletonJson_create(_atlas);
        //json->scale = 0.6f;
        spSkeletonData* skeletonData = spSkeletonJson_readSkeletonDataFile(json, skeletonFile.c_str());
        if (!skeletonData)
        {
            ouzel::log("Failed to load skeleton: %s", json->error);
            return;
        }
        spSkeletonJson_dispose(json);

        _bounds = spSkeletonBounds_create();

        _skeleton = spSkeleton_create(skeletonData);

        _animationStateData = spAnimationStateData_create(skeletonData);
        
        _animationState = spAnimationState_create(_animationStateData);
        _animationState->listener = listener;
        _animationState->rendererObject = this;

        _skeleton->flipX = false;
        _skeleton->flipY = false;
        spSkeleton_setToSetupPose(_skeleton);

        //_skeleton->x = 320;
        //_skeleton->y = -200;
        //spSkeleton_updateWorldTransform(_skeleton);

        _meshBuffer = ouzel::sharedEngine->getRenderer()->createMeshBuffer();
        _meshBuffer->setIndexSize(sizeof(uint16_t));
        _meshBuffer->setVertexAttributes(ouzel::graphics::VertexPCT::ATTRIBUTES);

        _blendState = ouzel::sharedEngine->getCache()->getBlendState(ouzel::graphics::BLEND_ALPHA);
        _shader = ouzel::sharedEngine->getCache()->getShader(ouzel::graphics::SHADER_TEXTURE);

        _updateCallback = std::make_shared<ouzel::UpdateCallback>();
        _updateCallback->callback = std::bind(&Skeleton::update, this, std::placeholders::_1);
        ouzel::sharedEngine->scheduleUpdate(_updateCallback);
    }

    Skeleton::~Skeleton()
    {
        if (_bounds)
        {
            spSkeletonBounds_dispose(_bounds);
        }

        if (_atlas)
        {
            spAtlas_dispose(_atlas);
        }

        if (_animationState)
        {
            spAnimationState_dispose(_animationState);
        }

        if (_animationStateData)
        {
            spAnimationStateData_dispose(_animationStateData);
        }

        if (_skeleton) spSkeleton_dispose(_skeleton);

        ouzel::sharedEngine->unscheduleUpdate(_updateCallback);
    }

    void Skeleton::update(float delta)
    {
        spSkeleton_update(_skeleton, delta);
        spAnimationState_update(_animationState, delta * _timeScale);
        spAnimationState_apply(_animationState, _skeleton);
        spSkeleton_updateWorldTransform(_skeleton);

        spSkeletonBounds_update(_bounds, _skeleton, true);

        _boundingBox.set(ouzel::Vector2(_bounds->minX, _bounds->minY),
                         ouzel::Vector2(_bounds->maxX, _bounds->maxY));
    }

    void Skeleton::draw(const ouzel::Matrix4& projection, const ouzel::Matrix4& transform, const ouzel::graphics::Color& color)
    {
        Drawable::draw(projection, transform, color);

        ouzel::sharedEngine->getRenderer()->activateBlendState(_blendState);
        ouzel::sharedEngine->getRenderer()->activateShader(_shader);

        ouzel::Matrix4 modelViewProj = projection * transform;
        float colorVector[] = { color.getR(), color.getG(), color.getB(), color.getA() };

        _shader->setVertexShaderConstant(0, sizeof(ouzel::Matrix4), 1, modelViewProj.m);
        _shader->setPixelShaderConstant(0, sizeof(colorVector), 1, colorVector);

        ouzel::graphics::VertexPCT vertex;

        uint16_t currentVertexIndex = 0;
        std::vector<uint16_t> indices;
        std::vector<ouzel::graphics::VertexPCT> vertices;

        ouzel::graphics::BlendStatePtr currentBlendState;
        uint32_t offset = 0;

        for (int i = 0; i < _skeleton->slotsCount; ++i)
        {
            spSlot* slot = _skeleton->drawOrder[i];
            spAttachment* attachment = slot->attachment;
            if (!attachment) continue;

            ouzel::graphics::BlendStatePtr blendState;
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
            if (!currentBlendState || currentBlendState != blendState)
            {
                if (indices.size() - offset > 0)
                {
                    _meshBuffer->uploadIndices(indices.data(), static_cast<uint32_t>(indices.size()));
                    _meshBuffer->uploadVertices(vertices.data(), static_cast<uint32_t>(vertices.size()));
                    ouzel::sharedEngine->getRenderer()->drawMeshBuffer(_meshBuffer, offset);
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
                spRegionAttachment_computeWorldVertices(regionAttachment, slot->bone, _worldVertices);

                uint8_t r = static_cast<uint8_t>(_skeleton->r * slot->r * 255);
                uint8_t g = static_cast<uint8_t>(_skeleton->g * slot->g * 255);
                uint8_t b = static_cast<uint8_t>(_skeleton->b * slot->b * 255);
                uint8_t a = static_cast<uint8_t>(_skeleton->a * slot->a * 255);

                vertex.color.r = r;
                vertex.color.g = g;
                vertex.color.b = b;
                vertex.color.a = a;
                vertex.position.x = _worldVertices[SP_VERTEX_X1];
                vertex.position.y = _worldVertices[SP_VERTEX_Y1];
                vertex.texCoord.x = regionAttachment->uvs[SP_VERTEX_X1];
                vertex.texCoord.y = regionAttachment->uvs[SP_VERTEX_Y1];
                vertices.push_back(vertex);

                vertex.color.r = r;
                vertex.color.g = g;
                vertex.color.b = b;
                vertex.color.a = a;
                vertex.position.x = _worldVertices[SP_VERTEX_X2];
                vertex.position.y = _worldVertices[SP_VERTEX_Y2];
                vertex.texCoord.x = regionAttachment->uvs[SP_VERTEX_X2];
                vertex.texCoord.y = regionAttachment->uvs[SP_VERTEX_Y2];
                vertices.push_back(vertex);

                vertex.color.r = r;
                vertex.color.g = g;
                vertex.color.b = b;
                vertex.color.a = a;
                vertex.position.x = _worldVertices[SP_VERTEX_X3];
                vertex.position.y = _worldVertices[SP_VERTEX_Y3];
                vertex.texCoord.x = regionAttachment->uvs[SP_VERTEX_X3];
                vertex.texCoord.y = regionAttachment->uvs[SP_VERTEX_Y3];
                vertices.push_back(vertex);

                vertex.color.r = r;
                vertex.color.g = g;
                vertex.color.b = b;
                vertex.color.a = a;
                vertex.position.x = _worldVertices[SP_VERTEX_X4];
                vertex.position.y = _worldVertices[SP_VERTEX_Y4];
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
                spMeshAttachment_computeWorldVertices(mesh, slot, _worldVertices);

                uint8_t r = static_cast<uint8_t>(_skeleton->r * slot->r * 255);
                uint8_t g = static_cast<uint8_t>(_skeleton->g * slot->g * 255);
                uint8_t b = static_cast<uint8_t>(_skeleton->b * slot->b * 255);
                uint8_t a = static_cast<uint8_t>(_skeleton->a * slot->a * 255);
                vertex.color.r = r;
                vertex.color.g = g;
                vertex.color.b = b;
                vertex.color.a = a;

                for (int t = 0; t < mesh->trianglesCount; ++t)
                {
                    int index = mesh->triangles[t] << 1;
                    vertex.position.x = _worldVertices[index];
                    vertex.position.y = _worldVertices[index + 1];
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
                spWeightedMeshAttachment_computeWorldVertices(mesh, slot, _worldVertices);
                
                uint8_t r = static_cast<uint8_t>(_skeleton->r * slot->r * 255);
                uint8_t g = static_cast<uint8_t>(_skeleton->g * slot->g * 255);
                uint8_t b = static_cast<uint8_t>(_skeleton->b * slot->b * 255);
                uint8_t a = static_cast<uint8_t>(_skeleton->a * slot->a * 255);
                vertex.color.r = r;
                vertex.color.g = g;
                vertex.color.b = b;
                vertex.color.a = a;

                for (int t = 0; t < mesh->trianglesCount; ++t)
                {
                    int index = mesh->triangles[t] << 1;
                    vertex.position.x = _worldVertices[index];
                    vertex.position.y = _worldVertices[index + 1];
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
            _meshBuffer->uploadIndices(indices.data(), static_cast<uint32_t>(indices.size()));
            _meshBuffer->uploadVertices(vertices.data(), static_cast<uint32_t>(vertices.size()));
            ouzel::sharedEngine->getRenderer()->drawMeshBuffer(_meshBuffer, offset);
        }
    }

    void Skeleton::setAnimation(int trackIndex, const std::string& animationName, bool loop)
    {
        spAnimationState_setAnimationByName(_animationState, trackIndex, animationName.c_str(), loop ? 1 : 0);
    }

    void Skeleton::addAnimation(int trackIndex, const std::string& animationName, bool loop, float delay)
    {
        spAnimationState_addAnimationByName(_animationState, trackIndex, animationName.c_str(), loop ? 1 : 0, delay);
    }

    void Skeleton::setAnimationMix(const std::string& from, const std::string& to, float duration)
    {
        // Configure mixing
        spAnimationStateData_setMixByName(_animationStateData, from.c_str(), to.c_str(), duration);
    }

    void Skeleton::setEventCallback(const std::function<void(int, spEventType, spEvent*, int)>& eventCallback)
    {
        _eventCallback = eventCallback;
    }

    void Skeleton::handleEvent(int trackIndex, spEventType type, spEvent* event, int loopCount)
    {
        if (_eventCallback)
        {
            _eventCallback(trackIndex, type, event, loopCount);
        }
    }
}
