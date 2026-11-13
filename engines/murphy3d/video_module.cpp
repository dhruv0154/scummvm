#include "murphy3d/video_module.h"
#include "common/algorithm.h"
#include "common/events.h"
#include "common/file.h"
#include "common/keyboard.h"
#include "common/system.h"
#include "graphics/cursorman.h"
#include "graphics/surface.h"
#include "image/png.h"
#include "murphy3d/animation_controller.h"
#include "murphy3d/archive.h"
#include "murphy3d/caption.h"
#include "murphy3d/game_controller.h"
#include "murphy3d/math_utils.h"
#include "murphy3d/module_controller.h"
#include "murphy3d/murphy3d.h"

namespace Murphy3d {

Texture *DialogueOption::_texBubble = nullptr;
Texture *ListBox::_texBackground = nullptr;
DialogueOption VideoModule::_dialogueOptions[3];
int VideoModule::_dialogueOptionsCount = 0;

DialogueOption::DialogueOption()
	: Control(), _pText(nullptr), _vbo(0), _boxWidth(0.0f), _boxHeight(0.0f),
	  _textX(0.0f), _value(0), _clicked(nullptr) {
}

DialogueOption::~DialogueOption() {
	clear();
}

void DialogueOption::initTexture() {
	if (_texBubble != nullptr)
		return;

	Common::File file;
	Image::PNGDecoder decoder;
	if (file.open(Common::Path("Bubble.png")) && decoder.loadStream(file)) {
		const Graphics::Surface *surf = decoder.getSurface();
		Graphics::PixelFormat rgbaFormat(4, 8, 8, 8, 8, 0, 8, 16, 24);
		Graphics::Surface *rgbaSurf = surf->convertTo(rgbaFormat);
		_texBubble = new Texture(rgbaSurf->w, rgbaSurf->h, (const byte *)rgbaSurf->getPixels());
		rgbaSurf->free();
		delete rgbaSurf;
		return;
	}

	byte rgba[64 * 64 * 4];
	for (int y = 0; y < 64; y++) {
		for (int x = 0; x < 64; x++) {
			int idx = (y * 64 + x) * 4;
			bool border = (x < 2 || x >= 62 || y < 2 || y >= 62);
			if (border) {
				rgba[idx + 0] = 32;
				rgba[idx + 1] = 32;
				rgba[idx + 2] = 32;
				rgba[idx + 3] = 240;
			} else {
				rgba[idx + 0] = 240;
				rgba[idx + 1] = 240;
				rgba[idx + 2] = 230;
				rgba[idx + 3] = 230;
			}
		}
	}
	_texBubble = new Texture(64, 64, rgba);
}

void DialogueOption::disposeTexture() {
	delete _texBubble;
	_texBubble = nullptr;
}

void DialogueOption::clear() {
	delete _pText;
	_pText = nullptr;

	if (_vbo != 0 && g_engine && g_engine->_renderer) {
		g_engine->_renderer->deleteBuffer(_vbo);
		_vbo = 0;
	}
}

void DialogueOption::setQuadVertex(Common::Array<TEXTURED_VERTEX> &vertices, float x1, float x2, float y1, float y2, float u1, float u2, float v1, float v2) {
	vertices.push_back({x1, y1, -0.5f, u1, v1});
	vertices.push_back({x2, y1, -0.5f, u2, v1});
	vertices.push_back({x2, y2, -0.5f, u2, v2});

	vertices.push_back({x1, y1, -0.5f, u1, v1});
	vertices.push_back({x2, y2, -0.5f, u2, v2});
	vertices.push_back({x1, y2, -0.5f, u1, v2});
}

void DialogueOption::setText(const Common::String &text, float boxWidth, float boxHeight, float x) {
	initTexture();
	clear();

	_textStr = text;
	_boxWidth = boxWidth;
	_boxHeight = boxHeight;

	_pText = new Text(g_engine ? g_engine->_font : nullptr);
	Rect rc = {0.0f, 0.0f, boxWidth, boxHeight};
	_pText->setText(text, rc, kAlignCenter, 1.0f);

	_textX = 32.0f;
	int screenH = g_system ? g_system->getHeight() : 480;
	_x = x;
	_y = (float)screenH - 64.0f;
	_w = boxWidth + 64.0f;
	_h = 64.0f;

	Common::Array<TEXTURED_VERTEX> vertices;
	float x1 = 0.0f;
	float x2 = 32.0f;
	float x3 = x2 + boxWidth;
	float x4 = x3 + 32.0f;
	float y1 = 0.0f;
	float y2 = -32.0f;
	float y3 = -64.0f;

	float u1 = 0.0f, u2 = 0.5f, u3 = 0.5f, u4 = 1.0f;
	float v1 = 1.0f, v2 = 0.5f, v3 = 0.0f;

	setQuadVertex(vertices, x1, x2, y1, y2, u1, u2, v1, v2);
	setQuadVertex(vertices, x2, x3, y1, y2, u2, u3, v1, v2);
	setQuadVertex(vertices, x3, x4, y1, y2, u3, u4, v1, v2);
	setQuadVertex(vertices, x1, x2, y2, y3, u1, u2, v2, v3);
	setQuadVertex(vertices, x2, x3, y2, y3, u2, u3, v2, v3);
	setQuadVertex(vertices, x3, x4, y2, y3, u3, u4, v2, v3);

	if (g_engine && g_engine->_renderer) {
		_vbo = g_engine->_renderer->createVertexBuffer(vertices.data(), vertices.size() * sizeof(TEXTURED_VERTEX), false);
	}
}

void DialogueOption::render() {
	if (_vbo == 0 || !g_engine || !g_engine->_renderer)
		return;

	initTexture();
	Math::Matrix4 wm = MathUtils::translation(_x, -_y, 0.0f);
	if (_texBubble != nullptr) {
		g_engine->_renderer->drawUITextured(_vbo, 36, _texBubble->getId(), wm);
	}

	if (_pText != nullptr) {
		uint32 mask = (_mouseOver && _enabled) ? 0xffffffff : 0x80ffffff;
		_pText->setColours(0x00000000 & mask, 0xff000000 & mask, 0xff000000 & mask, 0x00000000 & mask);
		float fontH = (g_engine->_font != nullptr) ? g_engine->_font->getHeight() : 14.0f;
		float textY = _y + 32.0f - (_pText->getLines() * fontH) / 2.0f;
		_pText->render(g_engine->_renderer, _textX + _x, textY, -0.5f);
	}
}

void DialogueOption::click() {
	if (_clicked != nullptr) {
		_clicked(nullptr);
	}
}

ListBox::ListBox()
	: Control(), _bgVbo(0), _hlVbo(0), _topIndex(0), _visibleItemsCount(0),
	  _highlightIndex(-1), _lineHeight(16.0f) {
}

ListBox::~ListBox() {
	clear();
}

void ListBox::initTexture() {
	if (_texBackground != nullptr)
		return;

	Common::File file;
	Image::PNGDecoder decoder;
	if (file.open(Common::Path("ListBox.png")) && decoder.loadStream(file)) {
		const Graphics::Surface *surf = decoder.getSurface();
		Graphics::PixelFormat rgbaFormat(4, 8, 8, 8, 8, 0, 8, 16, 24);
		Graphics::Surface *rgbaSurf = surf->convertTo(rgbaFormat);
		_texBackground = new Texture(rgbaSurf->w, rgbaSurf->h, (const byte *)rgbaSurf->getPixels());
		rgbaSurf->free();
		delete rgbaSurf;
		return;
	}

	byte rgba[64 * 64 * 4];
	for (int y = 0; y < 64; y++) {
		for (int x = 0; x < 64; x++) {
			int idx = (y * 64 + x) * 4;
			bool border = (x < 2 || x >= 46 || y < 16);
			if (border) {
				rgba[idx + 0] = 200;
				rgba[idx + 1] = 200;
				rgba[idx + 2] = 200;
				rgba[idx + 3] = 255;
			} else {
				rgba[idx + 0] = 24;
				rgba[idx + 1] = 24;
				rgba[idx + 2] = 32;
				rgba[idx + 3] = 235;
			}
		}
	}
	_texBackground = new Texture(64, 64, rgba);
}

void ListBox::disposeTexture() {
	delete _texBackground;
	_texBackground = nullptr;
}

void ListBox::clear() {
	for (uint i = 0; i < _entries.size(); i++) {
		delete _entries[i].label;
	}
	_entries.clear();

	if (g_engine && g_engine->_renderer) {
		if (_bgVbo != 0) {
			g_engine->_renderer->deleteBuffer(_bgVbo);
			_bgVbo = 0;
		}
		if (_hlVbo != 0) {
			g_engine->_renderer->deleteBuffer(_hlVbo);
			_hlVbo = 0;
		}
	}
}

void ListBox::setQuadVertex(Common::Array<TEXTURED_VERTEX> &vertices, float x1, float x2, float y1, float y2, float u1, float u2, float v1, float v2) {
	vertices.push_back({x1, y1, -0.4f, u1, v1});
	vertices.push_back({x2, y1, -0.4f, u2, v1});
	vertices.push_back({x2, y2, -0.4f, u2, v2});

	vertices.push_back({x1, y1, -0.4f, u1, v1});
	vertices.push_back({x2, y2, -0.4f, u2, v2});
	vertices.push_back({x1, y2, -0.4f, u1, v2});
}

void ListBox::init(const Common::Array<ListBoxItem> &items, float btnX) {
	initTexture();
	clear();

	Font *font = g_engine ? g_engine->_font : nullptr;
	float fontH = font ? font->getHeight() : 14.0f;
	_lineHeight = fontH + 2.0f;
	_w = 0.0f;

	for (uint i = 0; i < items.size(); i++) {
		Entry e;
		e.id = items[i].id;
		e.text = items[i].text;
		e.label = new Text(font);
		e.label->setText(e.text, 1.0f);

		float itemW = font ? font->getPixelWidth(e.text) : (e.text.size() * 8.0f);
		if (itemW > _w)
			_w = itemW;

		_entries.push_back(e);
	}

	int screenH = g_system ? g_system->getHeight() : 480;
	int maxVisible = MAX(1, (int)((screenH - 72.0f) / _lineHeight));
	_visibleItemsCount = MIN((int)_entries.size(), maxVisible);

	_w += 8.0f;
	float x1 = 0.0f;
	float x2 = 16.0f;
	float x3 = x2 + _w - 16.0f;
	float x4 = ceilf(x3 + 16.0f);
	float y1 = 0.0f;
	float y2 = -1.0f;
	float y3 = y2 - _lineHeight * (float)_visibleItemsCount;
	float y4 = y3 - 1.0f;

	_w = x4;

	float u1 = 0.0f, u2 = 0.25f, u3 = 0.5f, u4 = 0.75f;
	float v1 = 0.0f, v2 = 0.25f, v3 = 0.5f;

	Common::Array<TEXTURED_VERTEX> bgVerts;
	setQuadVertex(bgVerts, x1, x4, y1, y2, u1, u3, v1, v2);
	setQuadVertex(bgVerts, x1, x2, y2, y3, u1, u2, v3, v3);
	setQuadVertex(bgVerts, x2, x3, y2, y3, u2, u3, v3, v3);
	setQuadVertex(bgVerts, x3, x4, y2, y3, u3, u4, v3, v3);
	setQuadVertex(bgVerts, x1, x4, y3, y4, u1, u3, v1, v2);

	Common::Array<TEXTURED_VERTEX> hlVerts;
	setQuadVertex(hlVerts, x1, x2, 0.0f, -_lineHeight, u1, u2, v1, v2);
	setQuadVertex(hlVerts, x2, x3, 0.0f, -_lineHeight, u2, u3, v1, v2);
	setQuadVertex(hlVerts, x3, x4, 0.0f, -_lineHeight, u3, u4, v1, v2);

	if (g_engine && g_engine->_renderer) {
		_bgVbo = g_engine->_renderer->createVertexBuffer(bgVerts.data(), bgVerts.size() * sizeof(TEXTURED_VERTEX), false);
		_hlVbo = g_engine->_renderer->createVertexBuffer(hlVerts.data(), hlVerts.size() * sizeof(TEXTURED_VERTEX), false);
	}

	_h = 2.0f + (float)_visibleItemsCount * _lineHeight;
	_y = (float)screenH - _h - 64.0f;
	_topIndex = 0;
	_highlightIndex = -1;
	_x = floorf(MAX(0.0f, btnX - _w / 2.0f));
}

void ListBox::scroll(int delta) {
	if ((int)_entries.size() <= _visibleItemsCount)
		return;
	_topIndex = CLIP(_topIndex + delta, 0, (int)_entries.size() - _visibleItemsCount);
}

void ListBox::render() {
	if (_bgVbo == 0 || !g_engine || !g_engine->_renderer)
		return;

	initTexture();
	Math::Matrix4 wm = MathUtils::translation(_x, -_y, 0.0f);
	if (_texBackground != nullptr) {
		g_engine->_renderer->drawUITextured(_bgVbo, 30, _texBackground->getId(), wm);
	}

	if (_highlightIndex >= 0 && _highlightIndex < _visibleItemsCount && _hlVbo != 0 && _texBackground != nullptr) {
		Math::Matrix4 hlMat = MathUtils::translation(_x, -_y - 1.0f - (float)_highlightIndex * _lineHeight, -0.1f);
		g_engine->_renderer->drawUITextured(_hlVbo, 18, _texBackground->getId(), hlMat);
	}

	for (int i = 0; i < _visibleItemsCount; i++) {
		int idx = _topIndex + i;
		if (idx >= 0 && idx < (int)_entries.size() && _entries[idx].label != nullptr) {
			if (i == _highlightIndex) {
				_entries[idx].label->setColours(0x00000000, 0xff000000, 0xff000000, 0x00000000);
			} else {
				_entries[idx].label->setColours(0x00000000, 0xffc3c3c3, 0xffc3c3c3, 0x00000000);
			}
			float rowY = _y + 2.0f + (float)i * _lineHeight;
			_entries[idx].label->render(g_engine->_renderer, _x + 8.0f, rowY, -0.5f);
		}
	}
}

int ListBox::hitTestLB(float x, float y) {
	_highlightIndex = -1;

	if (x >= _x && x < (_x + _w) && y > _y && y < (_y + _h - 1.0f)) {
		_highlightIndex = static_cast<int>((y - _y) / _lineHeight);
		int selectedIndex = _topIndex + _highlightIndex;
		if (selectedIndex >= 0 && selectedIndex < (int)_entries.size()) {
			return _entries[selectedIndex].id;
		}
		_highlightIndex = -1;
	}

	return -1;
}

VideoModule::VideoModule(VideoType type, int dmapIndex, int activeScript)
	: ModuleBase(ModuleType::kVideo), _videoType(type), _scriptEngine(nullptr),
	  _scriptState(nullptr), _askAboutBase(0) {

	AnimationController::clear();
	AnimationController::clearImage();

	_scriptEngine = GameController::getScriptEngine();
	if (_scriptEngine != nullptr && ModuleController::_pDMap != nullptr) {
		_scriptEngine->_mapEntry = ModuleController::_pDMap->get(dmapIndex);
	}

	if (_scriptEngine != nullptr && _scriptEngine->_mapEntry != nullptr) {
		Common::String fileName = GameController::getFileName(_scriptEngine->_mapEntry->scriptFileIndex);
		if (!fileName.empty()) {
			Archive arch;
			if (arch.open(Common::Path(fileName))) {
				Common::SeekableReadStream *s = arch.getStream(_scriptEngine->_mapEntry->scriptFileEntry);
				if (s != nullptr) {
					uint32 len = s->size();
					uint8 *data = new uint8[len];
					s->read(data, len);
					delete s;

					_scriptState = GameController::getScriptState();
					if (_scriptState != nullptr) {
						_scriptState->init(data, len, fileName, _scriptEngine->_mapEntry->scriptFileEntry);
						if (activeScript < 0)
							activeScript = 0;
						_scriptState->_executionPointer = _scriptState->getScript(activeScript);
						if (_scriptState->_executionPointer < 0)
							_scriptState->_executionPointer = 0;
					} else {
						delete[] data;
					}
				}
			}
		}
	}
}

VideoModule::VideoModule(VideoType type, const Common::String &fileName, int itemIndex)
	: ModuleBase(ModuleType::kVideo), _videoType(type), _scriptEngine(nullptr),
	  _scriptState(nullptr), _askAboutBase(0) {

	_scriptState = GameController::getScriptState();
	AnimationController::load(fileName, itemIndex);
}

VideoModule::~VideoModule() {
	dispose();
}

void VideoModule::initialize() {
	if (g_system) {
		g_system->lockMouse(false);
		_cursorPosX = g_system->getWidth() / 2.0f;
		_cursorPosY = g_system->getHeight() / 2.0f;
	}

	_dialogueOptions[0].setClick(dialogueOptionA);
	_dialogueOptions[1].setClick(dialogueOptionB);
	_dialogueOptions[2].setClick(dialogueOptionC);

	if (_scriptEngine != nullptr && _scriptState != nullptr) {
		_scriptEngine->resume(_scriptState);
	}
}

void VideoModule::mouseMove(Common::Point pt) {
	cursor((float)pt.x, (float)pt.y, false);
}

void VideoModule::mouseMoveRel(Common::Point rel) {
	cursor((float)rel.x, (float)rel.y, true);
}

void VideoModule::mouseDown(Common::Point pt, int btn) {
	cursor((float)pt.x, (float)pt.y, false);
	if (btn == 1 || btn == 2) {
		beginAction();
	}
}

void VideoModule::mouseWheel(int scroll) {
	if (_scriptState != nullptr && _scriptState->_waitingForInput &&
		(_scriptState->_askAbout || _scriptState->_offer || _scriptState->_buy)) {
		_listBox.scroll(scroll > 0 ? -1 : 1);
	}
}

void VideoModule::keyDown(uint32 key, uint32 lParam) {
	ModuleBase::keyDown(key, lParam);

	if (key == Common::KEYCODE_ESCAPE) {
		back();
	} else if (key == Common::KEYCODE_RETURN || key == Common::KEYCODE_KP_ENTER || key == Common::KEYCODE_SPACE) {
		beginAction();
	}
}

void VideoModule::dispose() {
	delete _scriptEngine;
	_scriptEngine = nullptr;

	if (_scriptState != nullptr) {
		delete[] _scriptState->_script;
		_scriptState->_script = nullptr;
		delete _scriptState;
		_scriptState = nullptr;
	}
}

void VideoModule::disposeResources() {
	for (int i = 0; i < 3; i++) {
		_dialogueOptions[i].clear();
	}
	DialogueOption::disposeTexture();
	ListBox::disposeTexture();
}

void VideoModule::setDialogueOptions(const char *option1, const char *option2, const char *option3) {
	int options = (option3 && strlen(option3) > 0) ? 3 : ((option2 && strlen(option2) > 0) ? 2 : 1);
	setDialogueOptions(options, option1, 1, option2, 2, option3, 3);
}

void VideoModule::setDialogueOptions(int count, const char *option1, int value1, const char *option2, int value2, const char *option3, int value3) {
	_dialogueOptionsCount = count;
	if (count <= 0)
		return;

	int screenW = g_system ? g_system->getWidth() : 640;
	Font *font = g_engine ? g_engine->_font : nullptr;

	const char *opts[3] = {option1 ? option1 : "", option2 ? option2 : "", option3 ? option3 : ""};
	int vals[3] = {value1, value2, value3};

	float maxw = ((float)screenW - 64.0f * (float)count) / (float)count;
	float w = 0.0f;
	float h = 0.0f;

	for (int i = 0; i < count && i < 3; i++) {
		float ow = 0.0f, oh = 0.0f;
		if (font != nullptr) {
			font->getSize(opts[i], maxw, ow, oh, 1.0f);
		} else {
			ow = MIN(maxw, (float)strlen(opts[i]) * 8.0f);
			oh = 16.0f;
		}
		if (ow > w)
			w = ow;
		if (oh > h)
			h = oh;
	}

	float dow = w + 64.0f;
	float sx = ((float)screenW - (float)count * dow) / (float)count;

	if (count == 1) {
		_dialogueOptions[0].setText(opts[0], w, h, ((float)screenW - dow) / 2.0f);
		_dialogueOptions[0].setValue(vals[0]);
	} else if (count == 2) {
		float sx3 = sx / 3.0f;
		_dialogueOptions[0].setText(opts[0], w, h, sx3);
		_dialogueOptions[0].setValue(vals[0]);
		_dialogueOptions[1].setText(opts[1], w, h, (float)screenW - dow - sx3);
		_dialogueOptions[1].setValue(vals[1]);
	} else if (count >= 3) {
		_dialogueOptions[0].setText(opts[0], w, h, 0.0f);
		_dialogueOptions[0].setValue(vals[0]);
		_dialogueOptions[1].setText(opts[1], w, h, ((float)screenW - dow) / 2.0f);
		_dialogueOptions[1].setValue(vals[1]);
		_dialogueOptions[2].setText(opts[2], w, h, (float)screenW - dow);
		_dialogueOptions[2].setValue(vals[2]);
	}
}

void VideoModule::render() {
	if (g_engine && g_engine->_renderer) {
		g_engine->_renderer->clear(0.0f, 0.0f, 0.0f);
	}

	if (AnimationController::exists()) {
		AnimationController::updateAndRender();
	}

	bool animFinished = (!AnimationController::hasAnim() || AnimationController::isDone());

	if (_scriptState != nullptr && _scriptState->_waitingForInput && animFinished) {
		CursorMan.showMouse(true);

		for (int i = 0; i < _dialogueOptionsCount && i < 3; i++) {
			_dialogueOptions[i].render();
		}

		if (_scriptState->_askAbout || _scriptState->_offer || _scriptState->_buy) {
			bool recreate = ((_scriptState->_askAbout && GameController::_askAboutChanged) ||
							 (_scriptState->_offer && GameController::_itemsChanged) ||
							 (_scriptState->_buy && GameController::_buyChanged));

			if (_scriptState->_topItemOffset < 0 || recreate) {
				int valToFind = -1;
				Common::Array<ListBoxItem> items;

				if (_scriptState->_askAbout && !_scriptState->_askingAboutBuyables) {
					int askAboutCount = GameController::getAskAboutCount();
					for (int i = 0; i < 256 && i < askAboutCount; i++) {
						ListBoxItem lbi;
						lbi.id = GameController::getAskAboutId(i);
						lbi.text = GameController::getAskAboutName(lbi.id);
						items.push_back(lbi);
					}
					GameController::_askAboutChanged = false;
					valToFind = 4;
				} else if (_scriptState->_offer) {
					int itemCount = GameController::getItemCount();
					for (int i = 0; i < 256 && i < itemCount; i++) {
						ListBoxItem lbi;
						lbi.id = GameController::getItemId(i);
						lbi.text = GameController::getItemName(lbi.id);
						items.push_back(lbi);
					}
					GameController::_itemsChanged = false;
					valToFind = 7;
				}

				_scriptState->_topItemOffset = 0;
				int ix = 0;
				if (_dialogueOptions[1].getValue() == valToFind) {
					ix = 1;
				} else if (_dialogueOptions[2].getValue() == valToFind) {
					ix = 2;
				}
				_listBox.init(items, floorf(_dialogueOptions[ix].getX() + _dialogueOptions[ix].getWidth() / 2.0f));
			}

			_listBox.render();
		}

		ModuleController::_cursors[0].render();
	} else {
		CursorMan.showMouse(false);
	}

	if (animFinished) {
		if (_videoType == VideoType::kSingle) {
			AnimationController::clear();
			AnimationController::clearImage();
			ModuleController::pop(this);
		} else if (_videoType == VideoType::kScripted) {
			if (_scriptState == nullptr) {
				AnimationController::clear();
				AnimationController::clearImage();
				ModuleController::pop(this);
			} else if (_scriptState->_waitingForMediaToFinish) {
				_scriptEngine->resume(_scriptState, true);
				if (_scriptState != nullptr && _scriptState->_executionPointer == -1 &&
					!_scriptState->_waitingForInput && !_scriptState->_waitingForExternalModule &&
					!_scriptState->_waitingForMediaToFinish) {
					if (ModuleController::_nextModule == nullptr) {
						AnimationController::clear();
					}
					AnimationController::clearImage();
					ModuleController::pop(this);
				}
			} else if (_scriptState->_executionPointer == -1 && !_scriptState->_waitingForInput && !_scriptState->_waitingForExternalModule) {
				if (ModuleController::_nextModule == nullptr) {
					AnimationController::clear();
				}
				AnimationController::clearImage();
				ModuleController::pop(this);
			}
		}
	}
}

void VideoModule::dialogueOptionA(void *data) {
	(void)data;
	selectOption(_dialogueOptions[0].getValue());
}

void VideoModule::dialogueOptionB(void *data) {
	(void)data;
	selectOption(_dialogueOptions[1].getValue());
}

void VideoModule::dialogueOptionC(void *data) {
	(void)data;
	selectOption(_dialogueOptions[2].getValue());
}

void VideoModule::selectOption(int option) {
	if (ModuleController::_currentModule != nullptr && ModuleController::_currentModule->_type == ModuleType::kVideo) {
		static_cast<VideoModule *>(ModuleController::_currentModule)->selectDialogueOption(option);
	}
}

void VideoModule::selectDialogueOption(int option) {
	if (_scriptState != nullptr && _scriptEngine != nullptr) {
		_scriptState->_topItemOffset = -1;
		_scriptEngine->selectDialogueOption(_scriptState, option);
	}
}

void VideoModule::resize(int width, int height) {
	AnimationController::resize(width, height);
}

void VideoModule::resume() {
	if (g_system) {
		g_system->lockMouse(false);
	}
	if (_scriptState != nullptr && _scriptState->_waitingForExternalModule && _scriptEngine != nullptr) {
		_scriptEngine->resume(_scriptState, true);
	}
}

void VideoModule::cursor(float x, float y, bool relative) {
	ModuleBase::cursor(x, y, relative);

	if (_scriptState != nullptr && _scriptState->_waitingForInput) {
		x = _cursorPosX;
		y = _cursorPosY;

		for (int i = 0; i < 3; i++) {
			if (_dialogueOptionsCount > i) {
				_dialogueOptions[i].setMouseOver(_dialogueOptions[i].hitTest(x, y) != nullptr);
			}
		}

		if (_scriptState->_askAbout || _scriptState->_offer || _scriptState->_buy) {
			_listBox.hitTestLB(x, y);
		}
	}
}

void VideoModule::beginAction() {
	if (GameController::canCancelVideo()) {
		if (AnimationController::hasAnim()) {
			AnimationController::skip();
			Caption::clearCaptions(pDisplayCaptions);
			return;
		}

		float x = _cursorPosX;
		float y = _cursorPosY;

		if (_scriptState != nullptr && _scriptState->_waitingForInput) {
			if (_scriptState->_askAbout || _scriptState->_offer || _scriptState->_buy) {
				int hitId = _listBox.hitTestLB(x, y);
				if (hitId >= 0) {
					int option = _scriptState->_askAbout ? 4 : _scriptState->_offer ? 7
														   : _scriptState->_buy     ? 6
																					: -1;
					GameController::setSelectedItem(hitId + ((option == 4) ? _askAboutBase : 0));
					_scriptState->_selectedOption = option;
					_scriptState->_selectedValue = hitId + ((option == 4) ? _askAboutBase : 0);
					_scriptEngine->resume(_scriptState, true);
					return;
				}
			}

			for (int i = 0; i < 3; i++) {
				if (_dialogueOptionsCount > i && _dialogueOptions[i].hitTest(x, y) != nullptr) {
					_dialogueOptions[i].click();
					break;
				}
			}
		}
	}
}

void VideoModule::back() {
	ModuleBase *mainMenu = ModuleController::get(ModuleType::kMainMenu);
	if (mainMenu != nullptr) {
		ModuleController::sendToFront(mainMenu);
	}
}

} // End of namespace Murphy3d
