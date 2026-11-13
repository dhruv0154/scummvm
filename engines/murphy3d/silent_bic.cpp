#include "murphy3d/silent_bic.h"
#include "common/endian.h"

namespace Murphy3d {

SilentBIC::SilentBIC(int factor) : BIC(factor) {
}

bool SilentBIC::init(uint8 *pData, int length) {
	_embeddedAudioSize = 0;
	_firstAudioFrame = 0;
	_chunkTest = 0;

	AnimBase::init(pData, length);

	if (pData == nullptr || length < 0x30c)
		return false;

	_width = READ_LE_UINT16(pData + 6);
	_height = READ_LE_UINT16(pData + 8);
	_rate = 6;
	_frameTime = 100;

	createBuffers(_width, _height, _factor);

	byte *dstPal = (byte *)_pPalette;
	for (int c = 0; c < 256; c++) {
		double r = pData[12 + c * 3 + 0];
		double g = pData[12 + c * 3 + 1];
		double b = pData[12 + c * 3 + 2];
		dstPal[c * 4 + 0] = static_cast<uint8>((r * 255.0) / 63.0);
		dstPal[c * 4 + 1] = static_cast<uint8>((g * 255.0) / 63.0);
		dstPal[c * 4 + 2] = static_cast<uint8>((b * 255.0) / 63.0);
		dstPal[c * 4 + 3] = 255;
	}

	_videoFramePointer = 0x30c;
	_framePointer = _videoFramePointer;

	return true;
}

bool SilentBIC::decodeFrame() {
	bool ret = false;

	if (_pInputBuffer != nullptr && _framePointer >= 0 && _framePointer + 4 <= _inputBufferLength) {
		int chunkSize = (int32)READ_LE_UINT32(_pInputBuffer + _framePointer);
		if (chunkSize < 0 || _framePointer + 4 + chunkSize > _inputBufferLength) {
			_framePointer = _inputBufferLength;
			return false;
		}

		int end = processBICFrame(_framePointer + 4, chunkSize);
		if (end > _framePointer) {
			_framePointer += chunkSize + 4;
			ret = true;
		}
	}

	return ret;
}

} // End of namespace Murphy3d
