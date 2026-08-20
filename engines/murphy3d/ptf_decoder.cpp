#include "murphy3d/ptf_decoder.h"
#include "common/debug.h"
#include "audio/decoders/raw.h"

#define FLI_SS2 7
#define PTF_PALETTE 11
#define FLI_BLACK 13
#define FLI_BRUN 15

namespace Murphy3d {

PTFDecoder::PTFDecoder() : _stream(nullptr), _videoTrack(nullptr), _audioStream(nullptr), _audioFlags(0) {
}
		
PTFDecoder::~PTFDecoder() {
	if (_audioStream) {
		g_system->getMixer()->stopHandle(_audioHandle);
	}
}

bool PTFDecoder::loadStream(Common::SeekableReadStream* stream) {
	_stream = stream;

	// PTF header is 64 (0x40) bytes long
	if (_stream->size() < 0x40)
		return false;

	// 0x00
	uint32 fileLength = _stream->readUint32LE();

	// 0x04
	// read in BE so it matches the string ".PTF"
	uint32 magic = _stream->readUint32BE();
	if (magic != MKTAG('.', 'P', 'T', 'F')) {
		warning("PTFDecoder: Not a valid PTF file!");
		return false;
	}

	// 0x08 always 00 01
	_stream->skip(2);

	uint16 frameCount = _stream->readUint16LE();
	uint16 width = _stream->readUint16LE();
	uint16 height = _stream->readUint16LE();
	uint16 bpp = _stream->readUint16LE(); // should always be 8 for video
	uint16 rate = _stream->readUint16LE();

	debug(0, "PTFDecoder Loaded: %dx%d, %d Frames, Rate: %d", width, height, frameCount, rate);

	// create our custom video track
	// as the documentation notes, if width and height are 0, this is an audio only PTF
	// in that case, we simply don't create a video track
	if (width > 0 && height > 0) {
		_videoTrack = new PTFVideoTrack(_stream, frameCount, width, height, rate);
		addTrack(_videoTrack);
	}

	// the video/audio frame data doesn't begin until exactly offset 0x40
	_stream->seek(0x40);

	return true;
}

void PTFDecoder::readNextPacket() {
	if (!_stream || _stream->eos() || _stream->pos() >= _stream->size())
		return;

	while (!_stream->eos() && _stream->pos() < _stream->size()) {
		uint32 pos = _stream->pos();
		int32 chunkSize = _stream->readSint32LE();
		uint16 frameType = _stream->readUint16LE();

		if (_stream->eos())
			return;

		if (frameType == MKTAG16('V', 'W') || frameType == MKTAG16('W', 'V')) {
			byte *audioChunk = new byte[chunkSize];
			_stream->read(audioChunk, chunkSize);

			uint32 offset = 0;

			if (chunkSize >= 44 && READ_BE_UINT32(audioChunk) == MKTAG('R', 'I', 'F', 'F')) {
				uint16 channels = READ_LE_INT16(audioChunk + 0x16);
				uint32 sampleRate = READ_LE_UINT32(audioChunk + 0x18);
				uint16 bits = READ_LE_UINT16(audioChunk + 0x22);

				_audioFlags = Audio::FLAG_LITTLE_ENDIAN;
				if (channels == 2)
					_audioFlags |= Audio::FLAG_STEREO;

				if (bits == 16)
					_audioFlags |= Audio::FLAG_16BITS;
				else if (bits == 8)
					_audioFlags |= Audio::FLAG_UNSIGNED;
				

				if (!_audioStream) {
					_audioStream = Audio::makeQueuingAudioStream(sampleRate, channels == 2);
					g_system->getMixer()->playStream(Audio::Mixer::kPlainSoundType, &_audioHandle, _audioStream,
													-1, Audio::Mixer::kMaxChannelVolume, 0, DisposeAfterUse::YES);
				}
				offset = 44;
			}

			if (_audioStream) {
				uint32 audioDataSize = chunkSize - offset;
				byte *pcm = (byte *)malloc(audioDataSize);
				memcpy(pcm, audioChunk + offset, audioDataSize);

				_audioStream->queueBuffer(pcm, audioDataSize, DisposeAfterUse::YES, _audioFlags);
			}
			delete[] audioChunk;
			continue;
		}

		// if we hit a video chunk (0xF1FA or 0x0B1C) or an embedded palette (negative size),
		// we rewind the stream by 6 bytes and stop, decodeNextFrame will take it from here
		_stream->seek(pos);
		break;
	}
}

PTFDecoder::PTFVideoTrack::PTFVideoTrack(Common::SeekableReadStream *stream, uint16 frameCount, uint16 width, uint16 height, uint16 rate)
	: FlicVideoTrack(stream, frameCount, width, height, true) { // 'true' skips the FLC header

	// we must manually set the frame delay since we skipped the standard FLIC header
	_frameDelay = (100 * rate) / 6;
	_startFrameDelay = _frameDelay;
}

PTFDecoder::PTFVideoTrack::~PTFVideoTrack() {
}

const Graphics::Surface *PTFDecoder::PTFVideoTrack::decodeNextFrame() {
	if (!_fileStream || _fileStream->eos())
		return _surface;

	uint32 chunkPos = _fileStream->pos();
	int32 chunkSize = _fileStream->readSint32LE();
	uint16 frameType = _fileStream->readUint16LE();

	bool embeddedPalette = false;

	// if the chunk size is negative, an embedded palette precedes the frame data
	if (chunkSize < 0) {
		embeddedPalette = true;
		chunkSize = -chunkSize;

		byte paletteData[768];
		_fileStream->read(paletteData, 768);

		// convert from 6-bit (0-63) to 8-bit (0-255)
		for (int i = 0; i < 256; i++) {
			_palette.set(i,
						 (paletteData[i * 3 + 0] * 255) / 63,
						 (paletteData[i * 3 + 1] * 255) / 63,
						 (paletteData[i * 3 + 2] * 255) / 63);
		}
		_dirtyPalette = true;
	}

	if (frameType == 0xF1FA) {
		handleFrame();
	} else if (frameType == 0x0B1C) {
		decodeBicFrame(chunkSize);
	} else {
		debug(0, "PTF: Unknown Frame Type %04x, skipping.", frameType);
	}

	// ensure the head is perfectly positioned for the next chunk
	uint32 totalChunkLength = chunkSize + (embeddedPalette ? 768 : 0);

	// FLIC length includes the header, BIC/waveform length is data only
	if (frameType != 0xF1FA) {
		totalChunkLength += 6;
	}
	_fileStream->seek(chunkPos + totalChunkLength);

	_curFrame++;
	_nextFrameStartTime += _frameDelay;
	return _surface;
}

void PTFDecoder::PTFVideoTrack::handleFrame() {
	uint16 chunkCount = _fileStream->readUint16LE();
	uint16 newFrameDelay = _fileStream->readUint16LE();

	_fileStream->readUint16LE(); // reserved, always 0
	_fileStream->readUint16LE(); // width
	_fileStream->readUint16LE(); // height

	for (uint16 i = 0; i < chunkCount; i++) {
		uint32 frameSize = _fileStream->readUint32LE();
		uint16 frameType = _fileStream->readUint16LE();

		uint8 *data = new uint8[frameSize - 6];
		_fileStream->read(data, frameSize - 6);

		switch (frameType) {
		case FLI_SS2:
			decodeDeltaFLC(data);
			break;

		case PTF_PALETTE: {
			uint16 numPackets = READ_LE_UINT16(data);
			uint8 *p = data + 2;
			uint16 palPos = 0;

			while (numPackets-- > 0) {
				palPos += *p++;
				uint16 count = *p++;
				if (count == 0)
					count = 256;

				for (uint16 j = 0; j < count; j++) {
					byte r = (p[0] * 255) / 63;
					byte g = (p[1] * 255) / 63;
					byte b = (p[2] * 255) / 63;

					_palette.set((palPos + j) % 255, r, g, b);
					p += 3;
				}
				palPos += count;
			}
			_dirtyPalette = true;
			break;
		}
	
		case FLI_BLACK:
			_surface->fillRect(Common::Rect(0, 0, getWidth(), getHeight()), 0);
			_dirtyRects.clear();
			_dirtyRects.push_back(Common::Rect(0, 0, getWidth(), getHeight()));
			break;

		case FLI_BRUN:
			decodeByteRun(data);
			break;

		default:
			error("PTFDecoder::decodeNextFrame(): unknown subchunk type (type = 0x%02X)", frameType);
			break;

		}

		delete[] data;
	}
}

void PTFDecoder::PTFVideoTrack::decodeBicFrame(uint32 size) {
	uint32 startPos = _fileStream->pos();
	int currentRow = 0;

	while (_fileStream->pos() < startPos + size && currentRow < _surface->h) {
		byte type = _fileStream->readByte();

		if ((type & 1) != 0) {
			int currentX = 0;
			bool readOffset = 0;
			int16 chunks = _fileStream->readSint16LE();

			if (chunks < 0) {
				chunks = -chunks;
				readOffset = true;
			}

			if (chunks >= 160)
				break;

			while (chunks > 0) {
				if (readOffset) {
					currentX += _fileStream->readByte() * 4;
					chunks--;
				}

				byte count = _fileStream->readByte();

				for (int i = 0; i < count; i++) {
					byte color = _fileStream->readByte();

					for (int y = 0; y < 4; y++) {
						byte *pixel = (currentRow + y < _surface->h) ? (byte *)_surface->getBasePtr(currentX, currentRow + y) : nullptr;

						for (int x = 0; x < 4; x++) {
							if (pixel && currentX + x < _surface->w) {
								pixel[x] = color;
							}
						}
					}
					currentX += 4;
				}
				chunks--;
				readOffset = true;
			}
		}
		if ((type & 2) != 0) {
			int currentX = 0;
			bool readOffset = false;
			int16 chunks = _fileStream->readSint16LE();

			if (chunks < 0) {
				chunks = -chunks;
				readOffset = true;
			}

			if (chunks >= 160)
				break;

			while (chunks > 0) {
				if (readOffset) {
					currentX += _fileStream->readByte() * 4;
					chunks--;
				}

				byte count = _fileStream->readByte();

				for (int i = 0; i < count; i++) {
					byte c1 = _fileStream->readByte();
					byte c2 = _fileStream->readByte();

					uint16 pattern = _fileStream->readUint16LE();

					for (int y = 0; y < 4; y++) {
						byte *pixel = (currentRow + y < _surface->h) ? (byte *)_surface->getBasePtr(currentX, currentRow + y) : nullptr;

						if (pixel) {
							if (currentX + 0 < _surface->w)
								pixel[0] = ((pattern & 1) != 0) ? c2 : c1;
							if (currentX + 1 < _surface->w)
								pixel[1] = ((pattern & 2) != 0) ? c2 : c1;
							if (currentX + 2 < _surface->w)
								pixel[2] = ((pattern & 4) != 0) ? c2 : c1;
							if (currentX + 3 < _surface->w)
								pixel[3] = ((pattern & 8) != 0) ? c2 : c1;
						}
						pattern >>= 4;
					}
					currentX += 4;
				}
				chunks--;
				readOffset = true;
			}
		}
		if ((type & 4) != 0) {
			int currentX = 0;
			bool readOffset = false;
			int16 chunks = _fileStream->readSint16LE();

			if (chunks < 0) {
				chunks = -chunks;
				readOffset = true;
			}

			if (chunks >= 160)
				break;

			while (chunks > 0) {
				if (readOffset) {
					currentX += _fileStream->readByte() * 4;
					chunks--;
				}

				byte count = _fileStream->readByte();
				for (int i = 0; i < count; i++) {
					for (int y = 0; y < 4; y++) {
						byte *pixel = (currentRow + y < _surface->h) ? (byte *)_surface->getBasePtr(currentX, currentRow + y) : nullptr;

						for (int x = 0; x < 4; x++) {
							byte color = _fileStream->readByte();
							if (pixel && currentX + x < _surface->w) {
								pixel[x] = color;
							}
						}
					}
					currentX += 4;
				}
				chunks--;
				readOffset = true;
			}
		}
		if ((type & 8) != 0) {
			int currentX = 0;
			bool readOffset = false;
			int16 chunks = _fileStream->readSint16LE();

			if (chunks < 0) {
				chunks = -chunks;
				readOffset = true;
			}

			if (chunks >= 160)
				break;

			while (chunks > 0) {
				if (readOffset) {
					currentX += _fileStream->readByte() * 4;
					chunks--;
				}

				byte count = _fileStream->readByte();

				for (int i = 0; i < count; i++) {
					byte c = _fileStream->readByte();
					byte c2 = (c >> 4) & 0xf;
					byte c1 = c & 0xf;

					uint16 pattern = _fileStream->readUint16LE();

					for (int y = 0; y < 4; y++) {
						byte *pixel = (currentRow + y < _surface->h) ? (byte *)_surface->getBasePtr(currentX, currentRow + y) : nullptr;

						if (pixel) {
							if (currentX + 0 < _surface->w)
								pixel[0] = ((pattern & 1) != 0) ? c2 : c1;
							if (currentX + 1 < _surface->w)
								pixel[1] = ((pattern & 2) != 0) ? c2 : c1;
							if (currentX + 2 < _surface->w)
								pixel[2] = ((pattern & 4) != 0) ? c2 : c1;
							if (currentX + 3 < _surface->w)
								pixel[3] = ((pattern & 8) != 0) ? c2 : c1;
						}
						pattern >>= 4;
					}
					currentX += 4;
				}
				chunks--;
				readOffset = true;
			}
		}
		currentRow += 4;
	}

	_fileStream->seek(startPos + size);
}

} // End of namespace Murphy3d
