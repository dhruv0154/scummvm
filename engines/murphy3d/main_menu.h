#ifndef MURPHY3D_MAIN_MENU_H
#define MURPHY3D_MAIN_MENU_H

#include "common/array.h"
#include "common/rect.h"
#include "common/scummsys.h"
#include "common/str.h"
#include "murphy3d/button.h"
#include "murphy3d/font.h"
#include "murphy3d/module_base.h"
#include "murphy3d/texture.h"

namespace Murphy3d {

class Renderer;

class MainMenu : public ModuleBase {
public:
	MainMenu(Font *font = nullptr);
	~MainMenu() override;

	static MainMenu *_mainMenuModule;
	static void enableSaveAndResume(bool enable);

	void resize(int width, int height) override;
	void pause() override;
	void resume() override;
	void render() override;

	void mouseMove(Common::Point pt) override;
	void mouseDown(Common::Point pt, int btn) override;
	void mouseUp(Common::Point pt, int btn) override;
	void keyDown(uint32 key, uint32 lParam) override;

	void render(Renderer *renderer);
	void handleMouseMove(int x, int y);
	void handleMouseDown(int x, int y);
	void handleMouseUp(int x, int y);

	void enableResume(bool enable);
	void enableSave(bool enable);

protected:
	void initialize() override;

private:
	Font *_font;

	Texture *_bgTexture;
	GLuint _bgVbo;
	float _bgX, _bgY, _bgW, _bgH;

	Button *_btnNewGame;
	Button *_btnLoad;
	Button *_btnSave;
	Button *_btnConfig;
	Button *_btnIntro;
	Button *_btnCredits;
	Button *_btnResume;
	Button *_btnQuit;

	Common::Array<Button *> _buttons;
	Button *_hoveredButton;

	bool loadBackground();
	void buildBackgroundQuad(float w, float h);

	static void onNewGame(void *data);
	static void onLoad(void *data);
	static void onSave(void *data);
	static void onConfig(void *data);
	static void onIntro(void *data);
	static void onCredits(void *data);
	static void onResume(void *data);
	static void onQuit(void *data);
};

} // End of namespace Murphy3d

#endif // MURPHY3D_MAIN_MENU_H
