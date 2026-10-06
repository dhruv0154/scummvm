#include "murphy3d/main_menu.h"
#include "murphy3d/murphy3d.h"
#include "murphy3d/math_utils.h"
#include "common/file.h"
#include "common/debug.h"
#include "graphics/surface.h"
#include "image/png.h"
#include "image/jpeg.h"

namespace Murphy3d {

MainMenu::MainMenu(Font *font)
	: _font(font), _bgTexture(nullptr), _bgVbo(0),
	  _bgX(0.0f), _bgY(0.0f), _bgW(640.0f), _bgH(480.0f),
	  _btnNewGame(nullptr), _btnLoad(nullptr), _btnSave(nullptr),
	  _btnConfig(nullptr), _btnIntro(nullptr), _btnCredits(nullptr),
	  _btnResume(nullptr), _btnQuit(nullptr), _hoveredButton(nullptr) {
}

MainMenu::~MainMenu() {
	for (uint i = 0; i < _buttons.size(); ++i) {
		delete _buttons[i];
	}
	_buttons.clear();

	if (_bgTexture) {
		delete _bgTexture;
		_bgTexture = nullptr;
	}

	if (_bgVbo && g_engine && g_engine->_renderer) {
		g_engine->_renderer->deleteBuffer(_bgVbo);
		_bgVbo = 0;
	}

	Button::disposeTextures();
}

bool MainMenu::loadBackground() {
	Common::File *file = new Common::File();
	file->open(Common::Path("UAKM-Title.jpg"));

	if (!file) {
		warning("MainMenu: Failed to find background title image.");
		return false;
	}

	const Graphics::Surface *decodedSurface = nullptr;
	Image::JPEGDecoder jpgDecoder;

	if (jpgDecoder.loadStream(*file)) {
		decodedSurface = jpgDecoder.getSurface();
	}

	if (!decodedSurface) {
		warning("MainMenu: Decoder failed to parse title screen image.");
		delete file;
		return false;
	}

	Graphics::PixelFormat rgbaFormat(4, 8, 8, 8, 8, 0, 8, 16, 24);
	Graphics::Surface *rgbaSurf = decodedSurface->convertTo(rgbaFormat);

	_bgW = 640.0f;
	_bgH = 480.0f;
	_bgTexture = new Texture(rgbaSurf->w, rgbaSurf->h, (const byte *)rgbaSurf->getPixels());

	rgbaSurf->free();
	delete rgbaSurf;
	delete file;

	return true;
}

void MainMenu::buildBackgroundQuad(float w, float h) {
	Common::Array<TEXTURED_VERTEX> vertices;

	float x1 = 0.0f;
	float x2 = w;
	float y1 = 0.0f;
	float y2 = -h;

	float u1 = 0.0f, u2 = 1.0f;
	float v1 = 0.0f, v2 = 1.0f;

	vertices.push_back({x1, y1, 0.0f, u1, v1});
	vertices.push_back({x2, y1, 0.0f, u2, v1});
	vertices.push_back({x2, y2, 0.0f, u2, v2});

	vertices.push_back({x1, y1, 0.0f, u1, v1});
	vertices.push_back({x2, y2, 0.0f, u2, v2});
	vertices.push_back({x1, y2, 0.0f, u1, v2});

	if (g_engine && g_engine->_renderer) {
		_bgVbo = g_engine->_renderer->createVertexBuffer(vertices.data(), vertices.size() * sizeof(TEXTURED_VERTEX), false);
	}
}

bool MainMenu::init() {
	Button::initTextures("Button.png", "Button_MouseOver.png");

	loadBackground();
	if (_bgTexture) {
		buildBackgroundQuad(_bgW, _bgH);
	}

	const char *pNG = "New game";
	const char *pLG = "Load";
	const char *pSG = "Save";
	const char *pCf = "Config";
	const char *pIn = "Intro";
	const char *pCr = "Credits";
	const char *pRe = "Resume";
	const char *pQu = "Quit";

	float maxw = 0.0f;
	const char *labels[] = { pNG, pLG, pSG, pCf, pIn, pCr, pRe, pQu };
	for (int i = 0; i < 8; ++i) {
		float pw = _font->getPixelWidth(labels[i]);
		if (pw > maxw)
			maxw = pw;
	}

	float scale = 1.0f;
	float btnHeight = 32.0f * scale;

	float moonW = _bgW * 0.67f;
	float moonH = _bgH * 0.64f;
	float moonCenterX = _bgW - _bgX - moonW / 2.0f;
	float moonCenterY = _bgH / 2.2f;

	float btnRight  = moonCenterX + 32.0f * scale;
	float btnLeft   = btnRight - maxw - 96.0f * scale;
	float btnMiddle = moonCenterY;
	float btnTop    = btnMiddle - 64.0f * scale;
	float btnBottom = btnMiddle + 64.0f * scale;

	_btnNewGame = new Button(pNG, maxw, btnHeight, onNewGame, this, _font, scale);
	_btnNewGame->setPosition(btnLeft, btnTop);
	_buttons.push_back(_btnNewGame);

	_btnLoad = new Button(pLG, maxw, btnHeight, onLoad, this, _font, scale);
	_btnLoad->setPosition(btnLeft, btnMiddle);
	_buttons.push_back(_btnLoad);

	_btnSave = new Button(pSG, maxw, btnHeight, onSave, this, _font, scale);
	_btnSave->setPosition(btnLeft, btnBottom);
	_btnSave->setEnabled(false);
	_buttons.push_back(_btnSave);

	_btnConfig = new Button(pCf, maxw, btnHeight, onConfig, this, _font, scale);
	_btnConfig->setPosition(btnRight, btnTop);
	_buttons.push_back(_btnConfig);

	_btnIntro = new Button(pIn, maxw, btnHeight, onIntro, this, _font, scale);
	_btnIntro->setPosition(btnRight, btnMiddle);
	_buttons.push_back(_btnIntro);

	_btnCredits = new Button(pCr, maxw, btnHeight, onCredits, this, _font, scale);
	_btnCredits->setPosition(btnRight, btnBottom);
	_buttons.push_back(_btnCredits);

	float centerBtnX = moonCenterX - (maxw + 32.0f * scale) / 2.0f;
	_btnResume = new Button(pRe, maxw, btnHeight, onResume, this, _font, scale);
	_btnResume->setPosition(centerBtnX, btnTop - 64.0f * scale);
	_btnResume->setVisible(false);
	_buttons.push_back(_btnResume);

	_btnQuit = new Button(pQu, maxw, btnHeight, onQuit, this, _font, scale);
	_btnQuit->setPosition(centerBtnX, btnBottom + 64.0f * scale);
	_buttons.push_back(_btnQuit);

	return true;
}

void MainMenu::render(Renderer *renderer) {
	if (!renderer)
		return;

	if (_bgVbo && _bgTexture) {
		Math::Matrix4 bgWorld = MathUtils::translation(_bgX, -_bgY, 0.0f);
		renderer->drawUITextured(_bgVbo, 6, _bgTexture->getId(), bgWorld);
	}
	for (uint i = 0; i < _buttons.size(); ++i) {
		if (_buttons[i]->getVisible()) {
			_buttons[i]->render();
		}
	}
}

void MainMenu::handleMouseMove(int x, int y) {
	float fx = (float)x;
	float fy = (float)y;

	Button *hitBtn = nullptr;
	for (int i = (int)_buttons.size() - 1; i >= 0; --i) {
		if (_buttons[i]->getVisible() && _buttons[i]->getEnabled()) {
			if (_buttons[i]->hitTest(fx, fy)) {
				hitBtn = _buttons[i];
				break;
			}
		}
	}

	if (hitBtn != _hoveredButton) {
		if (_hoveredButton)
			_hoveredButton->mouseLeave();
		_hoveredButton = hitBtn;
		if (_hoveredButton)
			_hoveredButton->mouseEnter();
	}
}

void MainMenu::handleMouseDown(int x, int y) {
	float fx = (float)x;
	float fy = (float)y;

	for (uint i = 0; i < _buttons.size(); ++i) {
		if (_buttons[i]->getVisible() && _buttons[i]->getEnabled()) {
			if (_buttons[i]->hitTest(fx, fy)) {
				_buttons[i]->mouseButtonDown();
				break;
			}
		}
	}
}

void MainMenu::handleMouseUp(int x, int y) {
	float fx = (float)x;
	float fy = (float)y;

	for (uint i = 0; i < _buttons.size(); ++i) {
		if (_buttons[i]->getVisible() && _buttons[i]->getEnabled()) {
			if (_buttons[i]->hitTest(fx, fy)) {
				_buttons[i]->mouseButtonUp();
				break;
			}
		}
	}
}

void MainMenu::enableResume(bool enable) {
	if (_btnResume)
		_btnResume->setVisible(enable);
}

void MainMenu::enableSave(bool enable) {
	if (_btnSave)
		_btnSave->setEnabled(enable);
}

void MainMenu::onNewGame(void *data) {
	debug(0, "MainMenu: 'New Game' clicked.");
	if (g_engine) {
		g_engine->startNewGame();
	}
}

void MainMenu::onLoad(void *data) {
	debug(0, "MainMenu: 'Load' clicked.");
}

void MainMenu::onSave(void *data) {
	debug(0, "MainMenu: 'Save' clicked.");
}

void MainMenu::onConfig(void *data) {
	debug(0, "MainMenu: 'Config' clicked.");
}

void MainMenu::onIntro(void *data) {
	debug(0, "MainMenu: 'Intro' clicked.");
}

void MainMenu::onCredits(void *data) {
	debug(0, "MainMenu: 'Credits' clicked.");
}

void MainMenu::onResume(void *data) {
	debug(0, "MainMenu: 'Resume' clicked.");
	if (g_engine) {
		g_engine->resumeGame();
	}
}

void MainMenu::onQuit(void *data) {
	debug(0, "MainMenu: 'Quit' clicked.");
	if (g_engine) {
		g_engine->quitGame();
	}
}

} // End of namespace Murphy3d
