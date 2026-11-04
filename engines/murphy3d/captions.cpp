#include "common/debug.h"
#include "common/system.h"
#include "murphy3d/caption.h"
#include "murphy3d/murphy3d.h"

namespace Murphy3d {

Common::List<Caption *> Caption::_captions1;
Common::List<Caption *> Caption::_captions2;
Common::List<Caption *> *Caption::_pDisplayCaptions = &Caption::_captions1;
Common::List<Caption *> *Caption::_pAddCaptions = &Caption::_captions2;
Text Caption::_captionText;
bool Caption::_hasActiveText = false;

uint32 Caption::_texCaptionColour1 = 0xff000000;
uint32 Caption::_texCaptionColour2 = 0xff00c300;
uint32 Caption::_texCaptionColour3 = 0xff24ff00;
uint32 Caption::_texCaptionColour4 = 0xff000000;

uint32 Caption::_otherCaptionColour1 = 0xff000000;
uint32 Caption::_otherCaptionColour2 = 0xff0096ff;
uint32 Caption::_otherCaptionColour3 = 0xff00cfff;
uint32 Caption::_otherCaptionColour4 = 0xff000000;

void Caption::add(int frame, const Common::String &text, bool tex) {
	debug(0, "Caption::add(frame=%d, tex=%d): \"%s\"", frame, (int)tex, text.c_str());
	_pAddCaptions->push_back(new Caption(frame, text, tex));
}

void Caption::clearDisplayCaptions() {
	for (Caption *c : *_pDisplayCaptions) {
		delete c;
	}
	_pDisplayCaptions->clear();
	_captionText.setText("");
	_hasActiveText = false;
}

void Caption::swapCaptions() {
	Common::List<Caption *> *pOld = _pDisplayCaptions;
	_pDisplayCaptions = _pAddCaptions;
	_pAddCaptions = pOld;
	clearAddCaptions();
	_hasActiveText = false;
}

void Caption::clearAddCaptions() {
	for (Caption *c : *_pAddCaptions) {
		delete c;
	}
	_pAddCaptions->clear();
}

void Caption::clearAll() {
	clearDisplayCaptions();
	clearAddCaptions();
}

bool Caption::hasActiveCaption() {
	return !_pDisplayCaptions->empty() || _hasActiveText;
}

void Caption::updateAndRender(int currentFrame) {
	if (!g_engine || !g_engine->_renderer)
		return;

	Caption *pC = nullptr;
	for (Caption *cap : *_pDisplayCaptions) {
		if (cap->frame() <= currentFrame) {
			pC = cap;
		}
	}

	if (pC != nullptr && !pC->processed()) {
		Rect rc = {
			10.0f,
			0.0f,
			static_cast<float>(g_system->getWidth()) - 10.0f,
			static_cast<float>(g_system->getHeight())};
		_captionText.setText(pC->text(), rc, kAlignJustify, 1.0f);

		if (pC->tex()) {
			_captionText.setColours(_texCaptionColour1, _texCaptionColour2, _texCaptionColour3, _texCaptionColour4);
		} else {
			_captionText.setColours(_otherCaptionColour1, _otherCaptionColour2, _otherCaptionColour3, _otherCaptionColour4);
		}

		pC->setProcessed(true);
		_hasActiveText = (_captionText.getLines() > 0);
	}

	if (_hasActiveText) {
		float y = static_cast<float>(g_system->getHeight()) - _captionText.getHeight() - 10.0f;
		_captionText.render(g_engine->_renderer, 0.0f, y, -1.0f);
	}
}

} // End of namespace Murphy3d
