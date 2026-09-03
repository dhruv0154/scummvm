#include "murphy3d/texture.h"
#include "common/debug.h"

namespace Murphy3d {

Texture::Texture(uint16 width, uint16 height, const byte *rgbaData)
	: _width(width), _height(height), _glTextureId(0) {
	uploadToGPU(rgbaData);
}

Texture::Texture(uint16 width, uint16 height, const byte *indexedData, const uint32 *palette, bool transparent, bool rotated)
	: _glTextureId(0) {

	_width = rotated ? height : width;
	_height = rotated ? width : height;

	uint32 totalPixels = _width * _height;
	byte *rgbaData = new byte[totalPixels * 4];

	const byte *rawPaletteBytes = (const byte *)palette;

	// if the image is rotated we read column by column else row by row
	if (rotated) {
		for (uint16 y = 0; y < _height; y++) {
			for (uint16 x = 0; x < _width; x++) {
				byte colorIndex = indexedData[x * _height + y];
				uint32 dst = (y * _width + x) * 4;

				if (transparent && colorIndex == 0) {
					rgbaData[dst] = rgbaData[dst + 1] = rgbaData[dst + 2] = rgbaData[dst + 3] = 0;
				} else {
					rgbaData[dst + 0] = rawPaletteBytes[colorIndex * 4 + 0];
					rgbaData[dst + 1] = rawPaletteBytes[colorIndex * 4 + 1];
					rgbaData[dst + 2] = rawPaletteBytes[colorIndex * 4 + 2];
					rgbaData[dst + 3] = 255;
				}
			}
		}
	} else {
		for (uint16 y = 0; y < _height; y++) {
			for (uint16 x = 0; x < _width; x++) {
				byte colorIndex = indexedData[y * _width + x];
				uint32 dst = (y * _width + x) * 4;

				if (transparent && colorIndex == 0) {
					rgbaData[dst] = rgbaData[dst + 1] = rgbaData[dst + 2] = rgbaData[dst + 3] = 0;
				} else {
					rgbaData[dst + 0] = rawPaletteBytes[colorIndex * 4 + 0];
					rgbaData[dst + 1] = rawPaletteBytes[colorIndex * 4 + 1];
					rgbaData[dst + 2] = rawPaletteBytes[colorIndex * 4 + 2];
					rgbaData[dst + 3] = 255;
				}
			}
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
 
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaData);
	glBindTexture(GL_TEXTURE_2D, 0);
}

} // End of namespace Murphy3d
