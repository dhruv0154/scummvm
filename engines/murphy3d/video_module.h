#ifndef MURPHY3D_VIDEO_MODULE_H
#define MURPHY3D_VIDEO_MODULE_H

#include "common/array.h"
#include "common/scummsys.h"
#include "common/str.h"
#include "murphy3d/control.h"
#include "murphy3d/module_base.h"
#include "murphy3d/script_base.h"
#include "murphy3d/script_state.h"
#include "murphy3d/shader_structs.h"
#include "murphy3d/text.h"
#include "murphy3d/texture.h"

namespace Murphy3d {

enum class VideoType {
	kSingle = 1,
	kScripted = 2
};

struct ListBoxItem {
	int id;
	Common::String text;
};

class DialogueOption : public Control {
public:
	DialogueOption();
	~DialogueOption() override;

	static void initTexture();
	static void disposeTexture();

	void setClick(void (*clicked)(void *data)) { _clicked = clicked; }
	void setText(const Common::String &text, float boxWidth, float boxHeight, float x);

	void render() override;
	void click() override;
	void clear();

	void setValue(int value) { _value = value; }
	int getValue() const { return _value; }
	const Common::String &getText() const { return _textStr; }

private:
	static Texture *_texBubble;
	Text *_pText;
	GLuint _vbo;
	Common::String _textStr;
	float _boxWidth;
	float _boxHeight;
	float _textX;
	int _value;
	void (*_clicked)(void *data);

	void setQuadVertex(Common::Array<TEXTURED_VERTEX> &vertices, float x1, float x2, float y1, float y2, float u1, float u2, float v1, float v2);
};

class ListBox : public Control {
public:
	ListBox();
	~ListBox() override;

	static void initTexture();
	static void disposeTexture();

	void init(const Common::Array<ListBoxItem> &items, float btnX);
	void render() override;
	int hitTestLB(float x, float y);
	void scroll(int delta);
	void clear();

private:
	struct Entry {
		int id;
		Common::String text;
		Text *label;
	};

	static Texture *_texBackground;
	Common::Array<Entry> _entries;
	GLuint _bgVbo;
	GLuint _hlVbo;
	int _topIndex;
	int _visibleItemsCount;
	int _highlightIndex;
	float _lineHeight;

	void setQuadVertex(Common::Array<TEXTURED_VERTEX> &vertices, float x1, float x2, float y1, float y2, float u1, float u2, float v1, float v2);
};

class VideoModule : public ModuleBase {
public:
	VideoModule(VideoType type, int dmapIndex, int activeScript = -1);
	VideoModule(VideoType type, const Common::String &fileName, int itemIndex);
	~VideoModule() override;

	VideoType _videoType;

	void resize(int width, int height) override;
	void mouseMove(Common::Point pt) override;
	void mouseMoveRel(Common::Point rel) override;
	void mouseDown(Common::Point pt, int btn) override;
	void mouseWheel(int scroll) override;
	void keyDown(uint32 key, uint32 lParam) override;
	void dispose() override;
	void render() override;
	void resume() override;

	static void setDialogueOptions(const char *option1, const char *option2, const char *option3);
	static void setDialogueOptions(int count, const char *option1, int value1, const char *option2, int value2, const char *option3, int value3);
	static void disposeResources();

protected:
	void initialize() override;

	ScriptBase *_scriptEngine;
	ScriptState *_scriptState;
	ListBox _listBox;
	int _askAboutBase;

	static DialogueOption _dialogueOptions[3];
	static int _dialogueOptionsCount;

	static void dialogueOptionA(void *data);
	static void dialogueOptionB(void *data);
	static void dialogueOptionC(void *data);

	static void selectOption(int option);
	void selectDialogueOption(int option);

	void cursor(float x, float y, bool relative) override;
	void beginAction() override;
	void back() override;
};

} // End of namespace Murphy3d

#endif // MURPHY3D_VIDEO_MODULE_H
