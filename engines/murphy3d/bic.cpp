#include "murphy3d/bic.h"
#include "common/algorithm.h"
#include "common/endian.h"
#include "murphy3d/sound.h"
#include "murphy3d/uakm_script.h"

namespace Murphy3d {

BIC::BIC(int factor) : AnimBase(), _embeddedAudioSize(0), _firstAudioFrame(0), _chunkTest(0), _factor(factor) {
}

bool BIC::init(uint8 *pData, int length) {
	_embeddedAudioSize = 0;
	_firstAudioFrame = 0;
	_chunkTest = 0;

	AnimBase::init(pData, length);

	if (pData == nullptr || length < 0x340)
		return false;

	if (READ_LE_UINT32(pData) == 0x2b434942) {
		_width = READ_LE_UINT16(pData + 10);
		_height = READ_LE_UINT16(pData + 12);
		_rate = 6;
		_frameTime = 100;

		if (UAKMScript::_videoMode == VideoMode::kEmbedded && _width == 640) {
			_width = 432;
			_height = 324;
		}

		createBuffers(_width, _height, _factor);

		byte *dstPal = (byte *)_pPalette;
		for (int c = 0; c < 256; c++) {
			double r = pData[0x40 + c * 3 + 0];
			double g = pData[0x40 + c * 3 + 1];
			double b = pData[0x40 + c * 3 + 2];
			dstPal[c * 4 + 0] = static_cast<uint8>((r * 255.0) / 63.0);
			dstPal[c * 4 + 1] = static_cast<uint8>((g * 255.0) / 63.0);
			dstPal[c * 4 + 2] = static_cast<uint8>((b * 255.0) / 63.0);
			dstPal[c * 4 + 3] = 255;
		}

		_videoFramePointer = 0x340;
		_audioFramePointer = 0x340;

		_chunkTest = 10 - (int)READ_LE_INT16(pData + 0x12);
		_remainingAudioLength = (int)READ_LE_INT32(pData + 0x3c);
		_embeddedAudioSize = (int)READ_LE_UINT16(pData + 0x10);
		_firstAudioFrame = (int)READ_LE_INT16(pData + 0x12);

		if (_chunkTest > 0) {
			int preload = _chunkTest * _embeddedAudioSize;
			if (_videoFramePointer + preload <= length) {
				_videoFramePointer += preload;
			}
		}

		if (_audioFramePointer != 0 && _remainingAudioLength > 0) {
			AudioFormat fmt;
			fmt.channels = READ_LE_UINT16(_pInputBuffer + 0x2a);
			fmt.samplesPerSec = READ_LE_UINT32(_pInputBuffer + 0x2c);
			fmt.bitsPerSample = READ_LE_UINT16(_pInputBuffer + 0x36);

			_sourceVoice = Sound::createAudioStream(fmt);
			if (_sourceVoice != nullptr && _chunkTest > 0) {
				int audioBytes = _videoFramePointer - 0x340;
				if (audioBytes > 0 && 0x340 + audioBytes <= _inputBufferLength) {
					_remainingAudioLength -= audioBytes;
					_sourceVoice->submitBuffer(_pInputBuffer + 0x340, audioBytes);
					_audioFramesQueued++;
					if (_remainingAudioLength <= 0) {
						_sourceVoice->finish();
					}
				}
			}
		}

		_framePointer = _videoFramePointer;
		return true;
	}

	return false;
}

int BIC::processBICFrame(int inPtr, int chunkSize) {
	if (_pInputBuffer == nullptr || _pVideoOutputBuffer == nullptr || chunkSize <= 0)
		return inPtr;

	int outPtr = 0;
	int end = MIN(inPtr + chunkSize, _inputBufferLength);
	int currentRow = 0;
	int maxOut = _width * (((_height + 7) / 8) * 8);
	bool good = true;

	while (inPtr < end && good) {
		int type = _pInputBuffer[inPtr++];

		if ((type & 1) != 0) {
			if (inPtr + 2 > end)
				break;
			outPtr = currentRow * _width;

			bool readOffset = false;
			int chunks = (int16)READ_LE_UINT16(_pInputBuffer + inPtr);
			inPtr += 2;
			if (chunks < 0) {
				chunks = -chunks;
				readOffset = true;
			}

			if (chunks >= 160) {
				good = false;
				break;
			}

			while (chunks > 0 && inPtr < end) {
				if (readOffset) {
					outPtr += _pInputBuffer[inPtr++] * 4;
					chunks--;
				}
				if (inPtr >= end)
					break;

				int count = _pInputBuffer[inPtr++];
				for (int i = 0; i < count && inPtr < end; i++) {
					uint8 b = _pInputBuffer[inPtr++];
					int copyOut = outPtr;
					for (int y = 0; y < 4; y++) {
						if (copyOut >= 0 && copyOut + 4 <= maxOut) {
							for (int x = 0; x < 4; x++) {
								_pVideoOutputBuffer[copyOut + x] = b;
							}
						}
						copyOut += _width;
					}
					outPtr += 4;
				}

				chunks--;
				readOffset = true;
			}
		}

		if (good && (type & 2) != 0) {
			if (inPtr + 2 > end)
				break;
			outPtr = currentRow * _width;

			bool readOffset = false;
			int chunks = (int16)READ_LE_UINT16(_pInputBuffer + inPtr);
			inPtr += 2;
			if (chunks < 0) {
				chunks = -chunks;
				readOffset = true;
			}

			if (chunks >= 160) {
				good = false;
				break;
			}

			while (chunks > 0 && inPtr < end) {
				if (readOffset) {
					outPtr += _pInputBuffer[inPtr++] * 4;
					chunks--;
				}
				if (inPtr >= end)
					break;

				int count = _pInputBuffer[inPtr++];
				for (int i = 0; i < count && inPtr + 4 <= end; i++) {
					int c1 = _pInputBuffer[inPtr++];
					int c2 = _pInputBuffer[inPtr++];
					int pattern = (int16)READ_LE_UINT16(_pInputBuffer + inPtr);
					inPtr += 2;
					int copyOut = outPtr;
					for (int y = 0; y < 4; y++) {
						if (copyOut >= 0 && copyOut + 4 <= maxOut) {
							_pVideoOutputBuffer[copyOut + 0] = static_cast<uint8>(((pattern & 1) != 0) ? c2 : c1);
							_pVideoOutputBuffer[copyOut + 1] = static_cast<uint8>(((pattern & 2) != 0) ? c2 : c1);
							_pVideoOutputBuffer[copyOut + 2] = static_cast<uint8>(((pattern & 4) != 0) ? c2 : c1);
							_pVideoOutputBuffer[copyOut + 3] = static_cast<uint8>(((pattern & 8) != 0) ? c2 : c1);
						}
						copyOut += _width;
						pattern >>= 4;
					}
					outPtr += 4;
				}

				chunks--;
				readOffset = true;
			}
		}

		if (good && (type & 4) != 0) {
			if (inPtr + 2 > end)
				break;
			outPtr = currentRow * _width;

			bool readOffset = false;
			int chunks = (int16)READ_LE_UINT16(_pInputBuffer + inPtr);
			inPtr += 2;
			if (chunks < 0) {
				chunks = -chunks;
				readOffset = true;
			}

			if (chunks >= 160) {
				good = false;
				break;
			}

			while (chunks > 0 && inPtr < end) {
				if (readOffset) {
					outPtr += _pInputBuffer[inPtr++] * 4;
					chunks--;
				}
				if (inPtr >= end)
					break;

				int count = _pInputBuffer[inPtr++];
				for (int i = 0; i < count && inPtr + 16 <= end; i++) {
					int copyOut = outPtr;
					for (int y = 0; y < 4; y++) {
						for (int x = 0; x < 4; x++) {
							uint8 px = _pInputBuffer[inPtr++];
							if (copyOut + x >= 0 && copyOut + x < maxOut) {
								_pVideoOutputBuffer[copyOut + x] = px;
							}
						}
						copyOut += _width;
					}
					outPtr += 4;
				}

				chunks--;
				readOffset = true;
			}
		}

		if (good && (type & 8) != 0) {
			if (inPtr + 2 > end)
				break;
			outPtr = currentRow * _width;

			bool readOffset = false;
			int chunks = (int16)READ_LE_UINT16(_pInputBuffer + inPtr);
			inPtr += 2;
			if (chunks < 0) {
				chunks = -chunks;
				readOffset = true;
			}

			if (chunks >= 160) {
				good = false;
				break;
			}

			while (chunks > 0 && inPtr < end) {
				if (readOffset) {
					outPtr += _pInputBuffer[inPtr++] * 4;
					chunks--;
				}
				if (inPtr >= end)
					break;

				int count = _pInputBuffer[inPtr++];
				for (int i = 0; i < count && inPtr + 3 <= end; i++) {
					int c = _pInputBuffer[inPtr++];
					int c2 = (c >> 4) & 0xf;
					int c1 = c & 0xf;
					int pattern = (int16)READ_LE_UINT16(_pInputBuffer + inPtr);
					inPtr += 2;

					int copyOut = outPtr;
					for (int y = 0; y < 4; y++) {
						if (copyOut >= 0 && copyOut + 4 <= maxOut) {
							_pVideoOutputBuffer[copyOut + 0] = static_cast<uint8>(((pattern & 1) != 0) ? c2 : c1);
							_pVideoOutputBuffer[copyOut + 1] = static_cast<uint8>(((pattern & 2) != 0) ? c2 : c1);
							_pVideoOutputBuffer[copyOut + 2] = static_cast<uint8>(((pattern & 4) != 0) ? c2 : c1);
							_pVideoOutputBuffer[copyOut + 3] = static_cast<uint8>(((pattern & 8) != 0) ? c2 : c1);
						}
						copyOut += _width;
						pattern >>= 4;
					}
					outPtr += 4;
				}

				chunks--;
				readOffset = true;
			}
		}

		currentRow += 4;
	}

	return end;
}

bool BIC::decodeFrame() {
	bool ret = false;

	if (_pInputBuffer != nullptr && _framePointer >= 0 && _framePointer + 4 <= _inputBufferLength) {
		int chunkSize = (int32)READ_LE_UINT32(_pInputBuffer + _framePointer);
		if (chunkSize < 0 || _framePointer + 4 + chunkSize > _inputBufferLength) {
			_framePointer = _inputBufferLength;
			if (_sourceVoice != nullptr) {
				_sourceVoice->finish();
			}
			return false;
		}

		int end = processBICFrame(_framePointer + 4, chunkSize);

		if (end > _framePointer) {
			_framePointer += chunkSize + 4;

			_chunkTest++;
			if (_chunkTest >= 1 && _remainingAudioLength > 0) {
				int remainingBufferLength = _inputBufferLength - _framePointer;
				int waveChunkSize = (remainingBufferLength == _remainingAudioLength) ? _remainingAudioLength : MIN(_embeddedAudioSize, _remainingAudioLength);
				waveChunkSize = MIN(waveChunkSize, remainingBufferLength);

				if (waveChunkSize > 0) {
					_remainingAudioLength -= waveChunkSize;
					if (_sourceVoice != nullptr) {
						_sourceVoice->submitBuffer(_pInputBuffer + _framePointer, waveChunkSize);
						_audioFramesQueued++;
						if (_remainingAudioLength <= 0) {
							_sourceVoice->finish();
						}
					}
					_framePointer += waveChunkSize;
				} else {
					_remainingAudioLength = 0;
					if (_sourceVoice != nullptr) {
						_sourceVoice->finish();
					}
				}
			}

			if (_frame == _firstAudioFrame && _sourceVoice != nullptr) {
				_sourceVoice->start();
			}

			if (_framePointer >= _inputBufferLength && _sourceVoice != nullptr) {
				_sourceVoice->finish();
			}

			ret = true;
		}
	}

	return ret;
}

} // End of namespace Murphy3d
