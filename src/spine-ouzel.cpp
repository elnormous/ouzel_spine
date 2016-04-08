// Copyright (C) 2015 Elviss Strazdins

#include "spine-ouzel.h"
#include <fstream>

struct SpineTexture
{
    std::shared_ptr<ouzel::video::Texture> texture;
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

static void callback(spAnimationState* state, int trackIndex, spEventType type, spEvent* event, int loopCount)
{
    spTrackEntry* entry = spAnimationState_getCurrent(state, trackIndex);
    const char* animationName = (entry && entry->animation) ? entry->animation->name : 0;

    switch (type)
    {
        case SP_ANIMATION_START:
            printf("%d start: %s\n", trackIndex, animationName);
            break;
        case SP_ANIMATION_END:
            printf("%d end: %s\n", trackIndex, animationName);
            break;
        case SP_ANIMATION_COMPLETE:
            printf("%d complete: %s, %d\n", trackIndex, animationName, loopCount);
            break;
        case SP_ANIMATION_EVENT:
            printf("%d event: %s, %s: %d, %f, %s\n", trackIndex, animationName, event->data->name, event->intValue, event->floatValue,
                   event->stringValue);
            break;
    }
    fflush(stdout);
}

namespace spine
{
    SkeletonDrawable::SkeletonDrawable(const std::string& atlasFile, const std::string& skeletonFile)
    {
        spAtlas* atlas = spAtlas_createFromFile(atlasFile.c_str(), 0);
        spSkeletonJson* json = spSkeletonJson_create(atlas);
        //json->scale = 0.6f;
        spSkeletonData* skeletonData = spSkeletonJson_readSkeletonDataFile(json, skeletonFile.c_str());
        if (!skeletonData)
        {
            printf("%s\n", json->error);
            return;
        }
        spSkeletonJson_dispose(json);
        spSkeletonBounds* bounds = spSkeletonBounds_create();

        // Configure mixing.
        spAnimationStateData* stateData = spAnimationStateData_create(skeletonData);
        spAnimationStateData_setMixByName(stateData, "walk", "death", 0.5f);

        _skeleton = spSkeleton_create(skeletonData);

        stateData = spAnimationStateData_create(skeletonData);
        
        _state = spAnimationState_create(stateData);
        _state->listener = callback;

        _skeleton->flipX = false;
        _skeleton->flipY = false;
        spSkeleton_setToSetupPose(_skeleton);

        //_skeleton->x = 320;
        _skeleton->y = -200;
        spSkeleton_updateWorldTransform(_skeleton);

        spSkeletonBounds_update(bounds, _skeleton, true);

        spAnimationState_setAnimationByName(_state, 0, "walk", true);
        spAnimationState_addAnimationByName(_state, 0, "death", true, 5);

        _meshBuffer = ouzel::sharedEngine->getRenderer()->createMeshBuffer();
        _meshBuffer->setIndexSize(sizeof(uint16_t));
        _meshBuffer->setVertexAttributes(ouzel::video::VertexPCT::ATTRIBUTES);

        _blendState = ouzel::sharedEngine->getCache()->getBlendState(ouzel::video::BLEND_ALPHA);
        _shader = ouzel::sharedEngine->getCache()->getShader(ouzel::video::SHADER_TEXTURE);

#ifdef OUZEL_PLATFORM_WINDOWS
        _uniModelViewProj = 0;
#else
        _uniModelViewProj = _shader->getVertexShaderConstantId("modelViewProj");
#endif

        _updateCallback = std::make_shared<ouzel::UpdateCallback>();
        _updateCallback->callback = std::bind(&SkeletonDrawable::update, this, std::placeholders::_1);
        ouzel::sharedEngine->scheduleUpdate(_updateCallback);
    }

    SkeletonDrawable::~SkeletonDrawable()
    {
        if (_state)
        {
            if (_state->data) spAnimationStateData_dispose(_state->data);
            spAnimationState_dispose(_state);
        }

        if (_skeleton) spSkeleton_dispose(_skeleton);

        ouzel::sharedEngine->unscheduleUpdate(_updateCallback);
    }

    void SkeletonDrawable::update(float delta)
    {
        spSkeleton_update(_skeleton, delta);
        spAnimationState_update(_state, delta * _timeScale);
        spAnimationState_apply(_state, _skeleton);
        spSkeleton_updateWorldTransform(_skeleton);
    }

    void SkeletonDrawable::draw()
    {
        Node::draw();

        if (ouzel::scene::LayerPtr layer = _layer.lock())
        {
            ouzel::sharedEngine->getRenderer()->activateBlendState(_blendState);
            ouzel::sharedEngine->getRenderer()->activateShader(_shader);

            ouzel::Matrix4 modelViewProj = layer->getCamera()->getViewProjection() * _transform;

            _shader->setVertexShaderConstant(_uniModelViewProj, { modelViewProj });

            ouzel::video::VertexPCT vertex;

            uint16_t currentVertexIndex = 0;
            std::vector<uint16_t> indices;
            std::vector<ouzel::video::VertexPCT> vertices;

            for (int i = 0; i < _skeleton->slotsCount; ++i)
            {
                spSlot* slot = _skeleton->drawOrder[i];
                spAttachment* attachment = slot->attachment;
                if (!attachment) continue;

                /*sf::BlendMode blend;
                switch (slot->data->blendMode)
                {
                    case BLEND_MODE_ADDITIVE:
                        blend = BlendAdd;
                        break;
                    case BLEND_MODE_MULTIPLY:
                        blend = BlendMultiply;
                        break;
                    case BLEND_MODE_SCREEN: // Unsupported, fall through.
                    default:
                        blend = BlendAlpha;
                }
                if (states.blendMode != blend)
                {
                    target.draw(*vertexArray, states);
                    vertexArray->clear();
                    states.blendMode = blend;
                }*/

                SpineTexture* texture = 0;
                if (attachment->type == SP_ATTACHMENT_REGION)
                {
                    spRegionAttachment* regionAttachment = (spRegionAttachment*)attachment;
                    texture = (SpineTexture*)((spAtlasRegion*)regionAttachment->rendererObject)->page->rendererObject;
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
                    spMeshAttachment* mesh = (spMeshAttachment*)attachment;
                    if (mesh->verticesCount > SPINE_MESH_VERTEX_COUNT_MAX) continue;
                    texture = (SpineTexture*)((spAtlasRegion*)mesh->rendererObject)->page->rendererObject;
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
                    spWeightedMeshAttachment* mesh = (spWeightedMeshAttachment*)attachment;
                    if (mesh->uvsCount > SPINE_MESH_VERTEX_COUNT_MAX) continue;
                    texture = (SpineTexture*)((spAtlasRegion*)mesh->rendererObject)->page->rendererObject;
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

            _meshBuffer->uploadIndices(indices.data(), static_cast<uint32_t>(indices.size()));
            _meshBuffer->uploadVertices(vertices.data(), static_cast<uint32_t>(vertices.size()));
            ouzel::sharedEngine->getRenderer()->drawMeshBuffer(_meshBuffer);
        }
    }
}
