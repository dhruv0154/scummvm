#ifndef MURPHY3D_LABEL_H
#define MURPHY3D_LABEL_H

#include "murphy3d/control.h"
#include "murphy3d/text.h"

namespace Murphy3d {

class Label : public Control {
public:
	Label(const Common::String &text, Font *font, float scale = 1.0f);
	virtual ~Label() override;

	void render() override;
	void setText(const Common::String &text);
	void setColours(uint32 c1, uint32 c2 = 0, uint32 c3 = 0, uint32 c4 = 0);

private:
	Text *_text;
};

} // End of namespace Murphy3d
#endif
