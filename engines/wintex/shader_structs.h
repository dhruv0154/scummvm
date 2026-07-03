#ifndef WINTEX_SHADER_STRUCTS_H
#define WINTEX_SHADER_STRUCTS_H

#include "math/vector2d.h"
#include "math/vector3d.h"
#include "math/vector4d.h"
#include "math/matrix4.h"

namespace WinTex {
struct TEXTURED_VERTEX_ORTHO {
	Math::Vector3d position;
	Math::Vector2d texture;
};

struct TEXTURED_VERTEX {
	Math::Vector3d position;
	Math::Vector2d texture;
	Math::Vector2d object;           // Use for visibility, then create a visibility buffer for the objects and lookup in the shader
	Math::Vector4d objectParameters; // Use for triangle transparency indicator in shader (some textures are used both as opaque and transparent)
};

struct MULTICOLOURED_FONT_VERTEX {
	Math::Vector3d position;
	Math::Vector2d texture;
	Math::Vector4d colour;
};

struct COLOURED_VERTEX {
	Math::Vector4d position;
	Math::Vector4d colour;
	Math::Vector4d object;
};

struct COLOURED_VERTEX_ORTHO {
	Math::Vector4d position;
	Math::Vector4d colour;
};

struct VOPBufferType {
	Math::Matrix4 view;
	Math::Matrix4 ortho;
	Math::Matrix4 projection;
};

struct WorldBufferType {
	Math::Matrix4 world;
};

struct MultiColouredFontBufferType {
	Math::Vector4d colour1;
	Math::Vector4d colour2;
	Math::Vector4d colour3;
	Math::Vector4d colour4;
	Math::Vector4d colour5;
	Math::Vector4d colour6;
};

struct TexFontBufferType {
	Math::Vector4d colour1;
	Math::Vector4d colour2;
	Math::Vector4d colour3;
	Math::Vector4d colour4;
};

struct VisibilityBufferType {
	Math::Vector4d visibility[4096];
};

struct TranslationBufferType {
	Math::Vector4d translation[256];
};

} // End of namespace WinTex

#endif // WINTEX_SHADER_STRUCTS_H
