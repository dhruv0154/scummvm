#ifndef MURPHY3D_MODULE_CONTROLLER_H
#define MURPHY3D_MODULE_CONTROLLER_H

#include "common/list.h"
#include "common/rect.h"
#include "common/scummsys.h"
#include "murphy3d/animated_cursor.h"
#include "murphy3d/enums.h"
#include "murphy3d/map.h"
#include "murphy3d/module_base.h"

namespace Murphy3d {

class ModuleController {
public:
	static bool init(Map *map, Map *dmap);

	static void gotFocus();
	static void lostFocus();

	static void push(ModuleBase *pModule, bool overrideCurrent = false);
	static void pop(ModuleBase *pModule);
	static void sendToBack(ModuleBase *pModule);
	static void sendToFront(ModuleBase *pModule);

	static ModuleBase *get(ModuleType type);
	static void clearExcept(ModuleBase *pModule);

	static void render();

	static void mouseMove(Common::Point pt);
	static void mouseDown(Common::Point pt, int btn);
	static void mouseUp(Common::Point pt, int btn);
	static void mouseWheel(int scroll);
	static void keyDown(uint32 key, uint32 lParam);
	static void keyUp(uint32 key, uint32 lParam);

	static void resize(int width, int height);

	static AnimatedCursor _cursors[13];

	static ModuleBase *_currentModule;
	static ModuleBase *_nextModule;

	static Map *_pMap;
	static Map *_pDMap;

	static void resume();

protected:
	static Common::List<ModuleBase *> _modules;

	static void pauseModule(ModuleBase *pModule);
	static void resumeModule(ModuleBase *pModule);
};

} // End of namespace Murphy3d

#endif
