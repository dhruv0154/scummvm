#include "murphy3d/shader.h"
#include "common/debug.h"
#include "graphics/opengl/system_headers.h"

namespace Murphy3d {

ShaderManager::ShaderManager() : _texturedShader(nullptr) {
}

ShaderManager::~ShaderManager() {
	delete _texturedShader;
}

bool ShaderManager::init() {
	_texturedShader = new OpenGL::Shader();

	const char *const attributes[] = {
		"position",
		"texCoord",
		nullptr};

	bool success = _texturedShader->loadFromStrings("texturedShader", getVertexShaderTextured(),
					getPixelShaderTextured(), attributes, 120);

	if (!success) {
		warning("ShaderManager: Failed to compile Textured shader. %s", _texturedShader->getError().c_str());
		return false;
	}

	_texturedShader->use();
	_texturedShader->setUniform("texture1", 0);

	return true;
}

void ShaderManager::useTexturedShader() {
	if (_texturedShader)
		_texturedShader->use();
}

void ShaderManager::setMatrices(const Math::Matrix4 &world, const Math::Matrix4 &view, const Math::Matrix4 &proj) {
	if (!_texturedShader)
		return;
	_texturedShader->use();
	_texturedShader->setUniform("World", world);
	_texturedShader->setUniform("View", view);
	_texturedShader->setUniform("Projection", proj);
}

const char *ShaderManager::getVertexShaderTextured() {
	return "in vec3 position;\n"
		   "in vec2 texCoord;\n"
		   "out vec2 TexCoord;\n"
		   "uniform mat4 World;\n"
		   "uniform mat4 View;\n"
		   "uniform mat4 Projection;\n"
		   "void main() {\n"
		   "    gl_Position = Projection * View * World * vec4(position, 1.0);\n"
		   "    TexCoord = texCoord;\n"
		   "}\n";
}

const char *ShaderManager::getPixelShaderTextured() {
	return "in vec2 TexCoord;\n"
		   "OUTPUT\n"
		   "uniform sampler2D texture1;\n"
		   "void main() {\n"
		   "    vec4 texColor = texture(texture1, TexCoord);\n"
		   "    outColor = texColor;\n"
		   "}\n";
}

} // End of namespace Murphy3d
