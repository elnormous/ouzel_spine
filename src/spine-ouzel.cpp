// Copyright (C) 2015 Elviss Strazdins

#include "spine-ouzel.h"
#include <fstream>
#include <spine/extension.h>

#ifndef SPINE_MESH_VERTEX_COUNT_MAX
#define SPINE_MESH_VERTEX_COUNT_MAX 1000
#endif

struct SpineTexture
{
    std::shared_ptr<ouzel::video::Texture> texture;
};

void _spAtlasPage_createTexture (spAtlasPage* self, const char* path)
{
    SpineTexture* texture = new SpineTexture();

    texture->texture = ouzel::sharedEngine->getCache()->getTexture(path);

    self->width = static_cast<int>(texture->texture->getSize().width);
    self->height = static_cast<int>(texture->texture->getSize().height);
}

void _spAtlasPage_disposeTexture (spAtlasPage* self)
{
    delete static_cast<SpineTexture*>(self->rendererObject);
}

char* _spUtil_readFile (const char* path, int* length)
{
    return _readFile(ouzel::sharedEngine->getFileSystem()->getPath(path).c_str(), length);
}

namespace spine
{
    SkeletonDrawable::SkeletonDrawable (SkeletonData* skeletonData, AnimationStateData* stateData) :
				timeScale(1),
				vertexArray(new VertexArray(Triangles, skeletonData->bonesCount * 4)),
				worldVertices(0)
    {
        worldVertices = MALLOC(float, SPINE_MESH_VERTEX_COUNT_MAX);
        skeleton = Skeleton_create(skeletonData);

        ownsAnimationStateData = stateData == 0;
        if (ownsAnimationStateData) stateData = AnimationStateData_create(skeletonData);
        
        state = AnimationState_create(stateData);
    }

    SkeletonDrawable::~SkeletonDrawable () {
        delete vertexArray;
        FREE(worldVertices);
        if (ownsAnimationStateData) AnimationStateData_dispose(state->data);
        AnimationState_dispose(state);
        Skeleton_dispose(skeleton);
    }

    void SkeletonDrawable::update (float deltaTime) {
        Skeleton_update(skeleton, deltaTime);
        AnimationState_update(state, deltaTime * timeScale);
        AnimationState_apply(state, skeleton);
        Skeleton_updateWorldTransform(skeleton);
    }

    void SkeletonDrawable::draw(RenderTarget& target, RenderStates states) const {
        vertexArray->clear();

        sf::Vertex vertices[4];
        sf::Vertex vertex;
        for (int i = 0; i < skeleton->slotsCount; ++i) {
            Slot* slot = skeleton->drawOrder[i];
            Attachment* attachment = slot->attachment;
            if (!attachment) continue;

            sf::BlendMode blend;
            switch (slot->data->blendMode) {
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
            if (states.blendMode != blend) {
                target.draw(*vertexArray, states);
                vertexArray->clear();
                states.blendMode = blend;
            }

            Texture* texture = 0;
            if (attachment->type == ATTACHMENT_REGION) {
                RegionAttachment* regionAttachment = (RegionAttachment*)attachment;
                texture = (Texture*)((AtlasRegion*)regionAttachment->rendererObject)->page->rendererObject;
                RegionAttachment_computeWorldVertices(regionAttachment, slot->bone, worldVertices);

                Uint8 r = static_cast<Uint8>(skeleton->r * slot->r * 255);
                Uint8 g = static_cast<Uint8>(skeleton->g * slot->g * 255);
                Uint8 b = static_cast<Uint8>(skeleton->b * slot->b * 255);
                Uint8 a = static_cast<Uint8>(skeleton->a * slot->a * 255);

                Vector2u size = texture->getSize();
                vertices[0].color.r = r;
                vertices[0].color.g = g;
                vertices[0].color.b = b;
                vertices[0].color.a = a;
                vertices[0].position.x = worldVertices[VERTEX_X1];
                vertices[0].position.y = worldVertices[VERTEX_Y1];
                vertices[0].texCoords.x = regionAttachment->uvs[VERTEX_X1] * size.x;
                vertices[0].texCoords.y = regionAttachment->uvs[VERTEX_Y1] * size.y;

                vertices[1].color.r = r;
                vertices[1].color.g = g;
                vertices[1].color.b = b;
                vertices[1].color.a = a;
                vertices[1].position.x = worldVertices[VERTEX_X2];
                vertices[1].position.y = worldVertices[VERTEX_Y2];
                vertices[1].texCoords.x = regionAttachment->uvs[VERTEX_X2] * size.x;
                vertices[1].texCoords.y = regionAttachment->uvs[VERTEX_Y2] * size.y;

                vertices[2].color.r = r;
                vertices[2].color.g = g;
                vertices[2].color.b = b;
                vertices[2].color.a = a;
                vertices[2].position.x = worldVertices[VERTEX_X3];
                vertices[2].position.y = worldVertices[VERTEX_Y3];
                vertices[2].texCoords.x = regionAttachment->uvs[VERTEX_X3] * size.x;
                vertices[2].texCoords.y = regionAttachment->uvs[VERTEX_Y3] * size.y;

                vertices[3].color.r = r;
                vertices[3].color.g = g;
                vertices[3].color.b = b;
                vertices[3].color.a = a;
                vertices[3].position.x = worldVertices[VERTEX_X4];
                vertices[3].position.y = worldVertices[VERTEX_Y4];
                vertices[3].texCoords.x = regionAttachment->uvs[VERTEX_X4] * size.x;
                vertices[3].texCoords.y = regionAttachment->uvs[VERTEX_Y4] * size.y;

                vertexArray->append(vertices[0]);
                vertexArray->append(vertices[1]);
                vertexArray->append(vertices[2]);
                vertexArray->append(vertices[0]);
                vertexArray->append(vertices[2]);
                vertexArray->append(vertices[3]);

            } else if (attachment->type == ATTACHMENT_MESH) {
                MeshAttachment* mesh = (MeshAttachment*)attachment;
                if (mesh->verticesCount > SPINE_MESH_VERTEX_COUNT_MAX) continue;
                texture = (Texture*)((AtlasRegion*)mesh->rendererObject)->page->rendererObject;
                MeshAttachment_computeWorldVertices(mesh, slot, worldVertices);

                Uint8 r = static_cast<Uint8>(skeleton->r * slot->r * 255);
                Uint8 g = static_cast<Uint8>(skeleton->g * slot->g * 255);
                Uint8 b = static_cast<Uint8>(skeleton->b * slot->b * 255);
                Uint8 a = static_cast<Uint8>(skeleton->a * slot->a * 255);
                vertex.color.r = r;
                vertex.color.g = g;
                vertex.color.b = b;
                vertex.color.a = a;

                Vector2u size = texture->getSize();
                for (int i = 0; i < mesh->trianglesCount; ++i) {
                    int index = mesh->triangles[i] << 1;
                    vertex.position.x = worldVertices[index];
                    vertex.position.y = worldVertices[index + 1];
                    vertex.texCoords.x = mesh->uvs[index] * size.x;
                    vertex.texCoords.y = mesh->uvs[index + 1] * size.y;
                    vertexArray->append(vertex);
                }
                
            } else if (attachment->type == ATTACHMENT_WEIGHTED_MESH) {
                WeightedMeshAttachment* mesh = (WeightedMeshAttachment*)attachment;
                if (mesh->uvsCount > SPINE_MESH_VERTEX_COUNT_MAX) continue;
                texture = (Texture*)((AtlasRegion*)mesh->rendererObject)->page->rendererObject;
                WeightedMeshAttachment_computeWorldVertices(mesh, slot, worldVertices);
                
                Uint8 r = static_cast<Uint8>(skeleton->r * slot->r * 255);
                Uint8 g = static_cast<Uint8>(skeleton->g * slot->g * 255);
                Uint8 b = static_cast<Uint8>(skeleton->b * slot->b * 255);
                Uint8 a = static_cast<Uint8>(skeleton->a * slot->a * 255);
                vertex.color.r = r;
                vertex.color.g = g;
                vertex.color.b = b;
                vertex.color.a = a;
                
                Vector2u size = texture->getSize();
                for (int i = 0; i < mesh->trianglesCount; ++i) {
                    int index = mesh->triangles[i] << 1;
                    vertex.position.x = worldVertices[index];
                    vertex.position.y = worldVertices[index + 1];
                    vertex.texCoords.x = mesh->uvs[index] * size.x;
                    vertex.texCoords.y = mesh->uvs[index + 1] * size.y;
                    vertexArray->append(vertex);
                }
            }
            
            if (texture) {
                // SMFL doesn't handle batching for us, so we'll just force a single texture per skeleton.
                states.texture = texture;
            }
        }
        target.draw(*vertexArray, states);
    }
}
