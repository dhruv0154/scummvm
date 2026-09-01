#ifndef MURPHY3D_GRAPHICS_SHADER_STRUCTS_H
#define MURPHY3D_GRAPHICS_SHADER_STRUCTS_H

#include "math/matrix4.h"
#include "math/vector4d.h"

namespace Murphy3d {

struct Std140Vec4 {
	float x, y, z, w;
};

struct TEXTURED_VERTEX_ORTHO {
	float x, y, z; // position
	float u, v; // texture
};

struct TEXTURED_VERTEX {
	float x, y, z; // position
	float u, v; // texture
	float objX, objY; // object index & subIndex
	float pX, pY, pZ, pW; // object parameters
};

struct MULTICOLOURED_FONT_VERTEX {
	float x, y, z;
	float u, v;
	float r, g, b, a;
};

struct COLOURED_VERTEX {
	float x, y, z, w;
	float r, g, b, a;
	float oX, oY, oZ, oW;
};

struct COLOURED_VERTEX_ORTHO {
	float x, y, z, w;
	float r, g, b, a;
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
	Std140Vec4 colour1;
	Std140Vec4 colour2;
	Std140Vec4 colour3;
	Std140Vec4 colour4;
	Std140Vec4 colour5;
	Std140Vec4 colour6;
};

struct TexFontBufferType {
	Std140Vec4 colour1;
	Std140Vec4 colour2;
	Std140Vec4 colour3;
	Std140Vec4 colour4;
};

struct VisibilityBufferType {
	Std140Vec4 visibility[4096];
};

struct TranslationBufferType {
	Std140Vec4 translation[256];
};

} // End of namespace Murphy3d
#endif // MURPHY3D_GRAPHICS_SHADER_STRUCTS_H
