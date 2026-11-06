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
	OpenGL::Shader *getUIFontShader() const { return _uiFontShader; }
	OpenGL::Shader *getColouredShader() const { return _colouredShader; }

private:
	OpenGL::Shader *_texturedShader;
	OpenGL::Shader *_uiFontShader;
	OpenGL::Shader *_colouredShader;

	const char *getVertexShaderTextured();
	const char *getPixelShaderTextured();

	const char *getVertexShaderUIFont();
	const char *getPixelShaderUIFont();

	const char *getVertexShaderColoured();
	const char *getPixelShaderColoured();
};

} // End of namespace Murphy3d
#endif // MURPHY3D_GRAPHICS_SHADER_H
