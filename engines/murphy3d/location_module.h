#ifndef MURPHY3D_LOCATION_MODULE_H
#define MURPHY3D_LOCATION_MODULE_H

#include "common/rect.h"
#include "common/scummsys.h"
#include "common/str.h"
#include "murphy3d/location.h"
#include "murphy3d/module_base.h"
#include "murphy3d/script_base.h"
#include "murphy3d/script_state.h"
#include "murphy3d/text.h"

namespace Murphy3d {

#define MOVEMENT_WALK_SPEED 0.2f
#define MOVEMENT_RUN_SPEED 0.5f

class LocationModule : public ModuleBase {
public:
	LocationModule(int locationId, int startupPosition);
	~LocationModule() override;

	void resize(int width, int height) override;

	void pause() override;
	void resume() override;
	void render() override;

	static float _movement_left;
	static float _movement_right;
	static float _movement_forward;
	static float _movement_backward;
	static float _movement_x;
	static float _movement_y;
	static float _movement_z;
	static float _smooth_movement_x;
	static float _smooth_movement_z;
	static float _speed;

	static ActionType _currentAction;

	void keyDown(uint32 key, uint32 lParam) override;
	void keyUp(uint32 key, uint32 lParam) override;
	void mouseMove(Common::Point pt) override;
	void mouseDown(Common::Point pt, int btn) override;

protected:
	void initialize() override;

	int _actionColour1 = 0;
	int _actionColour2 = 0;
	int _actionColour3 = 0;
	int _actionColour4 = 0;
	int _currentActionColour1 = 0;
	int _currentActionColour2 = 0;
	int _currentActionColour3 = 0;
	int _currentActionColour4 = 0;
	int _unavailableActionColour1 = 0;
	int _unavailableActionColour2 = 0;
	int _unavailableActionColour3 = 0;
	int _unavailableActionColour4 = 0;

	int _locationId = 0;
	int _startupPosition = 0;

	Location _location;

	int _currentObjectIndex = -1;
	ActionType _currentActions = ActionType::kNone;
	int _currentActionMousePointerIndex = 0;
	Text _actionText[7];

	virtual void selectMouseAction();
	void cycleActions(bool allowUse);

	ScriptState *_environmentScriptState = nullptr;
	ScriptState *_actionScriptState = nullptr;
	ScriptState *_queryActionScriptState = nullptr;
	ScriptBase *_scriptEngine = nullptr;
	ScriptState *_initScriptState = nullptr;

	Common::Point _oldPoint;

	virtual void loadLocation(int locationFileIndex, uint8 *scriptData, int scriptLength, const Common::String &file, int entry);
	virtual void setLocationPosition(StartupPosition pos) {
		_location.setPosition(pos);
		_location.updateSprites();
	}

	void cursor(float x, float y, bool relative) override;
	void beginAction() override;
	void back() override;
	void cycle() override;
	void moveForward(float v) override;
	void moveBack(float v) override;
	void moveLeft(float v) override;
	void moveRight(float v) override;
	void moveUp(float y) override;
	void moveDown(float y) override;
	void run(bool isRunning) override;
	void next() override;
	void prev() override;
	void inventory() override;
	void travel() override;
	void hints() override;

	void cycleItems(int direction);
};

} // End of namespace Murphy3d

#endif
