#ifndef MURPHY3D_CAPTION_H
#define MURPHY3D_CAPTION_H

#include "common/list.h"
#include "common/scummsys.h"
#include "common/str.h"

namespace Murphy3d {

class Caption;

extern Common::List<Caption *> captions1;
extern Common::List<Caption *> captions2;
extern Common::List<Caption *> *pDisplayCaptions;
extern Common::List<Caption *> *pAddCaptions;

class Caption {
public:
	Caption() : _frame(-1), _text(""), _tex(false), _processed(false) {}

	Caption(int frame, const char *text, bool tex)
		: _frame(frame), _text(text ? text : ""), _tex(tex), _processed(false) {}

	~Caption() {}

	int frame() const { return _frame; }
	const char *text() const { return _text.c_str(); }
	bool tex() const { return _tex; }
	bool processed() const { return _processed; }
	void setProcessed(bool processed) { _processed = processed; }

	static Caption *getFrameCaption(int frame);
	static void addCaption(int frame, const char *text, bool tex);
	static void clearCaptions(Common::List<Caption *> *pCap);
	static void swapCaptions();
	static void destroy();

protected:
	int _frame;
	Common::String _text;
	bool _tex;
	bool _processed;
};

} // End of namespace Murphy3d

#endif // MURPHY3D_CAPTION_H
