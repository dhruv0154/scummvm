#ifndef MURPHY3D_GRAPHICS_TEXTURE_H
#define MURPHY3D_GRAPHICS_TEXTURE_H

#include "common/scummsys.h"
#include "graphics/opengl/system_headers.h"

namespace Murphy3d {

class Texture {
public:
	Texture(uint16 width, uint16 height, const byte *rgbaData);
	Texture(uint16 width, uint16 height, const byte *indexedData, const uint32 *palette, bool transparent, bool rotated);
	~Texture();

	GLuint getId() const { return _glTextureId; }
	uint16 getWidth() const { return _width; }
	uint16 getHeight() const { return _height; }

private:
	GLuint _glTextureId;
	uint16 _width;
	uint16 _height;

	void uploadToGPU(const byte *rgbaData);
};

} // End of namespace Murphy3d
#endif // MURPHY3D_GRAPHICS_TEXTURE_H
