#include "murphy3d/texture.h"
#include "common/debug.h"

namespace Murphy3d {

Texture::Texture(uint16 width, uint16 height, const byte *rgbaData)
	: _width(width), _height(height), _glTextureId(0) {
	uploadToGPU(rgbaData);
}

Texture::Texture(uint16 width, uint16 height, const byte *indexedData, const uint32 *palette, bool transparent)
	: _width(width), _height(height), _glTextureId(0) {

	uint32 totalPixels = _width * _height;
	byte *rgbaData = new byte[totalPixels * 4];

	const byte *rawPaletteBytes = (const byte *)palette;

	// convert 8 bit indexed pixels to 32 bit RGBA using the provided palette
	for (uint32 i = 0; i < totalPixels; i++) {
		byte colorIndex = indexedData[i];

		// in UAKM, index 0 is used as a transparent key if the texture is flagged transparent
		if (transparent && colorIndex == 0) {
			rgbaData[i * 4 + 0] = 0; // R
			rgbaData[i * 4 + 1] = 0; // G
			rgbaData[i * 4 + 2] = 0; // B
			rgbaData[i * 4 + 3] = 0; // A (fully transparent)
		} else {
			rgbaData[i * 4 + 0] = rawPaletteBytes[colorIndex * 4 + 0]; // R
			rgbaData[i * 4 + 1] = rawPaletteBytes[colorIndex * 4 + 1]; // G
			rgbaData[i * 4 + 2] = rawPaletteBytes[colorIndex * 4 + 2]; // B
			rgbaData[i * 4 + 3] = rawPaletteBytes[colorIndex * 4 + 3]; // A
		}
	}

	uploadToGPU(rgbaData);
	delete[] rgbaData;
}

Texture::~Texture() {
	if (_glTextureId)
		glDeleteTextures(1, &_glTextureId);
}

void Texture::uploadToGPU(const byte *rgbaData) {
	glGenTextures(1, &_glTextureId);
	glBindTexture(GL_TEXTURE_2D, _glTextureId);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaData);
	glBindTexture(GL_TEXTURE_2D, 0);
}

} // End of namespace Murphy3d
