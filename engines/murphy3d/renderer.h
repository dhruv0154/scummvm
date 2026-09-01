#ifndef MURPHY3D_GRAPHICS_RENDERER_H
#define MURPHY3D_GRAPHICS_RENDERER_H

#include "common/scummsys.h"
#include "math/matrix4.h"

#include "graphics/opengl/context.h"
#include "graphics/opengl/system_headers.h"

namespace Murphy3d {

class ShaderManager;

class Renderer {
public:
	Renderer();
	~Renderer();

	bool init();
	void clear(float r, float g, float b);

	GLuint createVertexBuffer(const void *data, uint32 size, bool dynamic = false);
	GLuint createIndexBuffer(const void *data, uint32 size, bool dynamic = false);
	GLuint createUniformBuffer(uint32 size);

	void updateBufferData(GLuint bufferId, const void *data, uint32 size, uint32 offset = 0);
	void deleteBuffer(GLuint bufferId);

	void updateVisibilityBuffer(const void *visibilityData, uint32 sizeInBytes);
	void updateTranslationBuffer(const void *translationData, uint32 sizeInBytes);

	void bindTexture(GLuint textureId, uint8 slot = 0);
	void updateMatrices(const Math::Matrix4 &world, const Math::Matrix4 &view, const Math::Matrix4 &proj);
	void drawTexturedTriangles(GLuint vbo, uint32 vertexCount, uint32 startVertex = 0);

private:
	GLuint _vao;
	GLuint _visibilityUbo;
	GLuint _translationUbo;

	ShaderManager *_shaderManager;
};

} // End of namespace Murphy3d

#endif
