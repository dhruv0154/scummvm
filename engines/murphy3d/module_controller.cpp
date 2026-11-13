#include "murphy3d/module_controller.h"
#include "common/system.h"
#include "murphy3d/murphy3d.h"
#include "murphy3d/renderer.h"

namespace Murphy3d {

ModuleBase *ModuleController::_currentModule = nullptr;
ModuleBase *ModuleController::_nextModule = nullptr;
Common::List<ModuleBase *> ModuleController::_modules;

AnimatedCursor ModuleController::_cursors[13];

Map *ModuleController::_pMap = nullptr;
Map *ModuleController::_pDMap = nullptr;

bool ModuleController::init(Map *map, Map *dmap) {
	if (map != nullptr) {
		_pMap = map;
		_pDMap = dmap;
		bool ret = map->init() && (dmap == nullptr || dmap->init());
		return ret;
	}

	return false;
}

void ModuleController::push(ModuleBase *pModule, bool overrideCurrent) {
	if (pModule != nullptr) {
		_nextModule = pModule;
		_modules.push_front(pModule);
		pModule->init();
	}
}

void ModuleController::pop(ModuleBase *pModule) {
	if (pModule != nullptr) {
		if (_currentModule == pModule) {
			_currentModule->pause();
		}

		_modules.remove(pModule);

		if (_currentModule == pModule) {
			if (_nextModule != nullptr) {
				_currentModule = nullptr;
			} else if (!_modules.empty()) {
				_currentModule = _modules.front();
				resumeModule(_currentModule);
			} else {
				_currentModule = nullptr;
			}
		}

		delete pModule;
	}
}

void ModuleController::destroy() {
	for (ModuleBase *it : _modules) {
		delete it;
	}
	_modules.clear();
	_currentModule = nullptr;
	_nextModule = nullptr;

	for (int i = 0; i < 13; ++i) {
		_cursors[i].dispose();
	}

	if (_pMap != nullptr) {
		delete _pMap;
		_pMap = nullptr;
	}

	if (_pDMap != nullptr) {
		delete _pDMap;
		_pDMap = nullptr;
	}
}

void ModuleController::sendToBack(ModuleBase *pModule) {
	if (pModule != nullptr) {
		_modules.remove(pModule);
		if (!_modules.empty()) {
			pModule->pause();
			_modules.push_back(pModule);
			_currentModule = _modules.front();
			_currentModule->resume();
		} else {
			if (g_engine != nullptr) {
				g_engine->quitGame();
			}
		}
	}
}

void ModuleController::sendToFront(ModuleBase *pModule) {
	if (pModule != nullptr) {
		pauseModule(_currentModule);
		_modules.remove(pModule);
		_modules.push_front(pModule);
		_currentModule = pModule;
		_currentModule->resume();
	}
}

void ModuleController::render() {
	if (_nextModule != nullptr) {
		pauseModule(_currentModule);

		Common::Array<ModuleBase *> modulesToDelete;
		for (ModuleBase *it : _modules) {
			if (it != nullptr && it->_type == _nextModule->_type && it != _nextModule) {
				modulesToDelete.push_back(it);
			}
		}

		for (ModuleBase *it : modulesToDelete) {
			_modules.remove(it);
			delete it;
		}

		_currentModule = _nextModule;
		_nextModule = nullptr;
	}

	if (_currentModule != nullptr) {
		g_engine->_renderer->clear(0, 0, 0);
		_currentModule->render();
		if (_currentModule != nullptr) {
			_currentModule->checkInput();
		}
	}
}

void ModuleController::mouseMove(Common::Point pt) {
	if (_currentModule != nullptr) {
		_currentModule->mouseMove(pt);
	}
}

void ModuleController::mouseDown(Common::Point pt, int btn) {
	if (_currentModule != nullptr) {
		_currentModule->mouseDown(pt, btn);
	}
}

void ModuleController::mouseMoveRel(Common::Point rel) {
	if (_currentModule != nullptr) {
		_currentModule->mouseMoveRel(rel);
	}
}

void ModuleController::mouseUp(Common::Point pt, int btn) {
	if (_currentModule != nullptr) {
		_currentModule->mouseUp(pt, btn);
	}
}

void ModuleController::mouseWheel(int scroll) {
	if (_currentModule != nullptr) {
		_currentModule->mouseWheel(scroll);
	}
}

void ModuleController::keyDown(uint32 key, uint32 lParam) {
	if (_currentModule != nullptr) {
		_currentModule->keyDown(key, lParam);
	}
}

void ModuleController::keyUp(uint32 key, uint32 lParam) {
	if (_currentModule != nullptr) {
		_currentModule->keyUp(key, lParam);
	}
}

void ModuleController::pauseModule(ModuleBase *pModule) {
	if (pModule != nullptr) {
		pModule->pause();
	}
}

void ModuleController::resumeModule(ModuleBase *pModule) {
	if (pModule != nullptr) {
		pModule->resume();
	}
}

ModuleBase *ModuleController::get(ModuleType type) {
	for (ModuleBase *it : _modules) {
		if (it != nullptr && it->_type == type) {
			return it;
		}
	}

	return nullptr;
}

void ModuleController::clearExcept(ModuleBase *pModule) {
	for (ModuleBase *it : _modules) {
		if (it != pModule && it != _currentModule) {
			delete it;
		}
	}

	_modules.clear();

	if (pModule != nullptr) {
		_modules.push_back(pModule);
	}

	if (_currentModule != nullptr && _currentModule != pModule) {
		_modules.push_back(_currentModule);
	}

	_currentModule = pModule;
}

void ModuleController::resize(int width, int height) {
	for (ModuleBase *it : _modules) {
		if (it != nullptr) {
			it->resize(width, height);
		}
	}
}

void ModuleController::gotFocus() {
	if (_currentModule != nullptr) {
		_currentModule->gotFocus();
	}
}

void ModuleController::lostFocus() {
	if (_currentModule != nullptr) {
		_currentModule->lostFocus();
	}
}

void ModuleController::resume() {
	if (_currentModule != nullptr) {
		_currentModule->resume();
	}
}

} // End of namespace Murphy3d
