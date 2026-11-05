#ifndef MURPHY3D_PTF_DECODER_H
#define MURPHY3D_PTF_DECODER_H

#include "common/rational.h"
#include "common/stream.h"
#include "graphics/surface.h"
#include "video/flic_decoder.h"
#include "audio/audiostream.h"
#include "audio/mixer.h"
#include "murphy3d/anim_base.h"

namespace Murphy3d {

class PTFDecoder : public AnimBase, public Video::FlicDecoder {

public:
	PTFDecoder(int factor = 1);
	virtual ~PTFDecoder() override;

	bool loadStream(Common::SeekableReadStream *stream);
	void readNextPacket() override;
	uint16 getRate() const { return _rate; }
	bool isStreamAtEnd() const { return !_stream || _stream->eos() || _stream->pos() >= _stream->size(); }
	bool isAudioPlaying() const;
	void stopAudio();

	bool init(uint8 *pData, int length) override;
	bool hasVideo() override { return _videoTrack != nullptr; }
	void skip() override;

protected:
	bool decodeFrame() override;

private:
	int _factor;
	Common::SeekableReadStream *_stream;

	Audio::QueuingAudioStream *_audioStream;
	Audio::SoundHandle _audioHandle;
	byte _audioFlags;
	uint16 _rate;

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

#endif // MURPHY3D_PTF_DECODER_H
