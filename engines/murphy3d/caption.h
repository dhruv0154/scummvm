#ifndef MURPHY3D_CAPTION_H
#define MURPHY3D_CAPTION_H

#include "common/list.h"
#include "common/scummsys.h"
#include "common/str.h"
#include "murphy3d/text.h"

namespace Murphy3d {

class Caption {
public:
	Caption() : _frame(-1), _text(""), _tex(false), _processed(false) {}
	Caption(int frame, const Common::String &text, bool tex)
		: _frame(frame), _text(text), _tex(tex), _processed(false) {}
	~Caption() {}

	int frame() const { return _frame; }
	const Common::String &text() const { return _text; }
	bool tex() const { return _tex; }
	bool processed() const { return _processed; }
	void setProcessed(bool processed) { _processed = processed; }

	// static caption list and rendering management
	static void add(int frame, const Common::String &text, bool tex);
	static void swapCaptions();
	static void clearDisplayCaptions();
	static void clearAddCaptions();
	static void clearAll();
	static bool hasActiveCaption();
	static void updateAndRender(int currentFrame = -1);

protected:
	int _frame;
	Common::String _text;
	bool _tex;
	bool _processed;

	static Common::List<Caption *> _captions1;
	static Common::List<Caption *> _captions2;
	static Common::List<Caption *> *_pDisplayCaptions;
	static Common::List<Caption *> *_pAddCaptions;
	static Text _captionText;
	static bool _hasActiveText;

	static uint32 _texCaptionColour1;
	static uint32 _texCaptionColour2;
	static uint32 _texCaptionColour3;
	static uint32 _texCaptionColour4;

	static uint32 _otherCaptionColour1;
	static uint32 _otherCaptionColour2;
	static uint32 _otherCaptionColour3;
	static uint32 _otherCaptionColour4;
};

} // End of namespace Murphy3d

#endif
