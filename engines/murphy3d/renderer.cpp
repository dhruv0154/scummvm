#include "common/system.h"
#include "murphy3d/renderer.h"
#include "murphy3d/shader.h"
#include "murphy3d/shader_structs.h"

namespace Murphy3d {

Renderer::Renderer() : _shaderManager(nullptr) {
}

Renderer::~Renderer() {
	if (_shaderManager)
		delete _shaderManager;
}

bool Renderer::init() {
#if !defined(USE_OPENGL_SHADERS)
	return false;
#else

	// initGraphics3d() creates the context and initializes OpenGLContext.
	// Do not request a different context type here: the backend-selected
	// compatibility context is also used by ScummVM's shader renderers.
	if (!OpenGLContext.enginesShadersSupported)
		return false;

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_CULL_FACE);

	_shaderManager = new ShaderManager();
	if (!_shaderManager->init()) {
		warning("Renderer: Failed to initialize ShaderManager.");
		return false;
	}

	return true;
#endif
}

void Renderer::clear(float r, float g, float b) {
	glViewport(0, 0, g_system->getWidth(), g_system->getHeight());
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);  
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glClearColor(r, g, b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

GLuint Renderer::createVertexBuffer(const void *data, uint32 size, bool dynamic) {
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glBufferData(GL_ARRAY_BUFFER, size, data, dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	return vbo;
}

GLuint Renderer::createIndexBuffer(const void *data, uint32 size, bool dynamic) {
	GLuint ebo;
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	return ebo;
}

void Renderer::updateBufferData(GLuint bufferId, const void *data, uint32 size, uint32 offset) {
	glBindBuffer(GL_ARRAY_BUFFER, bufferId);
	glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
}

void Renderer::deleteBuffer(GLuint bufferId) {
	glDeleteBuffers(1, &bufferId);
}

void Renderer::updateMatrices(const Math::Matrix4 &world, const Math::Matrix4 &view, const Math::Matrix4 &proj) {
	if (_shaderManager) {
		_shaderManager->setMatrices(world, view, proj);
	}
}

void Renderer::bindTexture(GLuint textureId, uint8 slot) {
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, textureId);
}

void Renderer::drawTexturedTriangles(GLuint vbo, uint32 vertexCount, uint32 startVertex) {
	if (!_shaderManager)
		return;

	OpenGL::Shader *shader = _shaderManager->getTexturedShader();
	GLsizei stride = sizeof(TEXTURED_VERTEX);

	shader->enableVertexAttribute("position", vbo, 3, GL_FLOAT, GL_FALSE, stride, 0);
	shader->enableVertexAttribute("texCoord", vbo, 2, GL_FLOAT, GL_FALSE, stride, 3 * sizeof(float));
	shader->use(true);

	glDrawArrays(GL_TRIANGLES, startVertex, vertexCount);
}

} // End of namespace Murphy3d
