#include "murphy3d/caption.h"

namespace Murphy3d {

Common::List<Caption *> captions1;
Common::List<Caption *> captions2;
Common::List<Caption *> *pDisplayCaptions = &captions1;
Common::List<Caption *> *pAddCaptions = &captions2;

Caption *Caption::getFrameCaption(int frame) {
	if (!pDisplayCaptions)
		return nullptr;

	for (Common::List<Caption *>::iterator it = pDisplayCaptions->begin(); it != pDisplayCaptions->end(); ++it) {
		if ((*it)->frame() == frame)
			return *it;
	}
	return nullptr;
}

void Caption::addCaption(int frame, const char *text, bool tex) {
	if (pAddCaptions) {
		pAddCaptions->push_back(new Caption(frame, text, tex));
	}
}

void Caption::clearCaptions(Common::List<Caption *> *pCap) {
	if (!pCap)
		return;
	for (Common::List<Caption *>::iterator it = pCap->begin(); it != pCap->end(); ++it) {
		delete *it;
	}
	pCap->clear();
}

void Caption::swapCaptions() {
	Common::List<Caption *> *pOld = pDisplayCaptions;
	pDisplayCaptions = pAddCaptions;
	pAddCaptions = pOld;
	clearCaptions(pOld);
}

void Caption::destroy() {
	clearCaptions(&captions1);
	clearCaptions(&captions2);
}

} // End of namespace Murphy3d
