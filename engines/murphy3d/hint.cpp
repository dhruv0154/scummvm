#include "murphy3d/hint.h"

namespace Murphy3d {

Hint::Hint(int hintIndex, const Common::String &text)
	: _hintIndex(hintIndex), _text(text), _state(0) {
}

Hint::~Hint() {
}

HintCategory::HintCategory(int categoryIndex, const Common::String &title)
	: _hintCategoryIndex(categoryIndex), _title(title) {
}

HintCategory::~HintCategory() {
	for (Hint *h : _hints) {
		delete h;
	}
	_hints.clear();
}

void HintCategory::addHint(int hintIndex, const Common::String &text) {
	_hints.push_back(new Hint(hintIndex, text));
}

void HintCategory::reset() {
	for (Hint *h : _hints) {
		h->setState(0);
	}
}

} // End of namespace Murphy3d
