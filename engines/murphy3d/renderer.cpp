#include "murphy3d/renderer.h"
#include "murphy3d/shader.h"
#include "murphy3d/shader_structs.h"

namespace Murphy3d {

Renderer::Renderer() : _vao(0), _visibilityUbo(0), _translationUbo(0), _shaderManager(nullptr) {
}

Renderer::~Renderer() {
	if (_vao)
		glDeleteVertexArrays(1, &_vao);
	if (_visibilityUbo)
		glDeleteBuffers(1, &_visibilityUbo);
	if (_translationUbo)
		glDeleteBuffers(1, &_translationUbo);
	if (_shaderManager)
		delete _shaderManager;
}

bool Renderer::init() {
#if !defined(USE_OPENGL_GAME)
	return false;
#endif

	OpenGLContext.initialize(OpenGL::ContextType::kContextGL);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_CULL_FACE);

	glGenVertexArrays(1, &_vao);
	glBindVertexArray(_vao);

	_visibilityUbo = createUniformBuffer(4096 * 16);
	_translationUbo = createUniformBuffer(256 * 16);

	glBindBufferBase(GL_UNIFORM_BUFFER, 3, _visibilityUbo);
	glBindBufferBase(GL_UNIFORM_BUFFER, 5, _translationUbo);

	_shaderManager = new ShaderManager();
	if (!_shaderManager->init()) {
		warning("Renderer: Failed to initialize ShaderManager.");
		return false;
	}

	return true;
}

void Renderer::clear(float r, float g, float b) {
	glBindVertexArray(_vao);
	glViewport(0, 0, 640, 480);
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

GLuint Renderer::createUniformBuffer(uint32 size) {
	GLuint ubo;
	glGenBuffers(1, &ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);

	glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	return ubo;
}

void Renderer::updateBufferData(GLuint bufferId, const void *data, uint32 size, uint32 offset) {
	glBindBuffer(GL_ARRAY_BUFFER, bufferId);
	glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
}

void Renderer::updateVisibilityBuffer(const void *visibilityData, uint32 sizeInBytes) {
	glBindBuffer(GL_UNIFORM_BUFFER, _visibilityUbo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeInBytes, visibilityData);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Renderer::updateTranslationBuffer(const void *translationData, uint32 sizeInBytes) {
	glBindBuffer(GL_UNIFORM_BUFFER, _translationUbo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeInBytes, translationData);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
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
	shader->enableVertexAttribute("object", vbo, 2, GL_FLOAT, GL_FALSE, stride, 5 * sizeof(float));
	shader->enableVertexAttribute("objectParameters", vbo, 4, GL_FLOAT, GL_FALSE, stride, 7 * sizeof(float));
	shader->use();

	glDrawArrays(GL_TRIANGLES, startVertex, vertexCount);
}

} // End of namespace Murphy3d
