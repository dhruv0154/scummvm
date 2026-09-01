#ifndef MURPHY3D_GRAPHICS_SHADER_H
#define MURPHY3D_GRAPHICS_SHADER_H

#include "graphics/opengl/shader.h"
#include "math/matrix4.h"

namespace Murphy3d {

class ShaderManager {
public:
	ShaderManager();
	~ShaderManager();

	bool init();
	void useTexturedShader();
	void setMatrices(const Math::Matrix4 &world, const Math::Matrix4 &view, const Math::Matrix4 &proj);

	OpenGL::Shader *getTexturedShader() const { return _texturedShader; }

private:
	OpenGL::Shader *_texturedShader;

	const char *getVertexShaderTextured();
	const char *getPixelShaderTextured();
};

} // End of namespace Murphy3d
#endif // MURPHY3D_GRAPHICS_SHADER_H
