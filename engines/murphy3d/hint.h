#ifndef MURPHY3D_HINT_H
#define MURPHY3D_HINT_H

#include "common/list.h"
#include "common/rect.h"
#include "common/str.h"

namespace Murphy3d {

class Hint {
public:
	Hint(int hintIndex, const Common::String &text);
	~Hint();

	int getHintIndex() const { return _hintIndex; }
	int getState() const { return _state; }
	void setState(int state) { _state = state; }
	const Common::String &getText() const { return _text; }

protected:
	int _hintIndex;
	Common::String _text;
	int _state;
};

class HintCategory {
public:
	HintCategory(int categoryIndex, const Common::String &title);
	~HintCategory();

	void addHint(int hintIndex, const Common::String &text);
	void reset();

	int getCategoryIndex() const { return _hintCategoryIndex; }
	const Common::String &getTitle() const { return _title; }

	Common::List<Hint *> _hints;

private:
	int _hintCategoryIndex;
	Common::String _title;
};

} // End of namespace Murphy3d

#endif // MURPHY3D_HINT_H
