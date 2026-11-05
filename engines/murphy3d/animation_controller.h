#ifndef MURPHY3D_ANIMATION_CONTROLLER_H
#define MURPHY3D_ANIMATION_CONTROLLER_H

#include "murphy3d/anim_base.h"
#include "murphy3d/text.h"

namespace Murphy3d {

class AnimationController {
public:
	static void init();
	static void clear();

	static bool load(const Common::String &fileName, int itemIndex);
	static bool skip();
	static bool isWave();
	static bool hasAnim();
	static bool noAnimOrWave();
	static bool updateAndRender(bool render = true);
	static bool updateAndRender(AnimBase *pAnim, bool render = true);
	static bool isDone();
	static bool animNotDoneOrCondition(bool condition);
	static bool noVideoAnim();
	static int frame();
	static int frame(AnimBase *pAnim);
	static int exists() { return (_anim != nullptr); }
	static void resize(int width, int height);

	static void setOutputBuffer(uint8 *pBuffer, int width, int height, int offsetX, int offsetY, int *pPalette, int minColAllowChange, int maxColAllowChange);

	static AnimBase *load(uint8 *data, int length, int factor = 1);

	static int width() { return (_anim != nullptr) ? _anim->width() : 0; }
	static int height() { return (_anim != nullptr) ? _anim->height() : 0; }

	static void setCaptionColours(int texColour1, int texColour2, int texColour3, int texColour4, int otherColour1, int otherColour2, int otherColour3, int otherColour4);

	static void renderCaptions(float z);

protected:
	static AnimBase *_anim;
	static Text *_pCaption;

	static int _texCaptionColour1;
	static int _texCaptionColour2;
	static int _texCaptionColour3;
	static int _texCaptionColour4;
	static int _otherCaptionColour1;
	static int _otherCaptionColour2;
	static int _otherCaptionColour3;
	static int _otherCaptionColour4;
};

} // End of namespace Murphy3d

#endif // MURPHY3D_ANIMATION_CONTROLLER_H
