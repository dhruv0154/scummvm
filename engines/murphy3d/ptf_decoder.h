#ifndef MURPHY3D_PTF_DECODER_H
#define MURPHY3D_PTF_DECODER_H

#include "common/rational.h"
#include "common/stream.h"
#include "graphics/surface.h"
#include "video/flic_decoder.h"
#include "audio/audiostream.h"
#include "audio/mixer.h"

namespace Murphy3d {

class PTFDecoder : public Video::FlicDecoder {

public:
	PTFDecoder();
	virtual ~PTFDecoder() override;

	bool loadStream(Common::SeekableReadStream *stream);

protected:
	void readNextPacket() override;

private:
	Common::SeekableReadStream *_stream;

	Audio::QueuingAudioStream *_audioStream;
	Audio::SoundHandle _audioHandle;
	byte _audioFlags;

	class PTFVideoTrack : public Video::FlicDecoder::FlicVideoTrack {
	public:
		PTFVideoTrack(Common::SeekableReadStream *stream, uint16 frameCount, uint16 width, uint16 height, uint16 rate);
		virtual ~PTFVideoTrack() override;

		const Graphics::Surface *decodeNextFrame() override;
		void handleFrame() override;

		void decodeBicFrame(uint32 size);
	};

	PTFVideoTrack *_videoTrack;
};

} // End of namespace Murphy3d

#endif
