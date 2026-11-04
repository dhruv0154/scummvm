#include "murphy3d/uakm_script.h"
#include "common/debug.h"
#include "murphy3d/game_controller.h"
#include "murphy3d/location_module.h"
#include "murphy3d/module_controller.h"
#include "murphy3d/uakm_game.h"
#include "murphy3d/animation_controller.h"
#include "murphy3d/ambient_audio.h"

namespace Murphy3d {

UAKMScript::UAKMScript() {
	_functions[0x00] = &UAKMScript::function_80;
	_functions[0x01] = &UAKMScript::function_81;
	_functions[0x02] = &UAKMScript::function_82;
	_functions[0x03] = &UAKMScript::function_83;
	_functions[0x04] = &UAKMScript::function_84;
	_functions[0x05] = &UAKMScript::function_85;
	_functions[0x06] = &UAKMScript::function_86;
	_functions[0x07] = &UAKMScript::function_87;
	_functions[0x08] = &UAKMScript::function_88;
	_functions[0x09] = &UAKMScript::function_89;
	_functions[0x0A] = &UAKMScript::function_8A;
	_functions[0x0B] = &UAKMScript::function_8B;
	_functions[0x0C] = &UAKMScript::function_8C;
	_functions[0x0D] = &UAKMScript::function_8D;
	_functions[0x0E] = &UAKMScript::function_8E;
	_functions[0x0F] = &UAKMScript::function_8F;
	_functions[0x10] = &UAKMScript::function_90;
	_functions[0x11] = &UAKMScript::function_91;
	_functions[0x12] = &UAKMScript::function_92;
	_functions[0x13] = &UAKMScript::function_93;
	_functions[0x14] = &UAKMScript::function_94;
	_functions[0x15] = &UAKMScript::function_95;
	_functions[0x16] = &UAKMScript::function_96;
	_functions[0x17] = &UAKMScript::function_97;
	_functions[0x18] = &UAKMScript::function_98;
	_functions[0x19] = &UAKMScript::function_99;
	_functions[0x1A] = &UAKMScript::function_9A;
	_functions[0x1B] = &UAKMScript::function_9B;
	_functions[0x1C] = &UAKMScript::function_9C;
	_functions[0x1D] = &UAKMScript::function_9D;
	_functions[0x1E] = &UAKMScript::function_9E;
	_functions[0x1F] = &UAKMScript::function_9F;
	_functions[0x20] = &UAKMScript::function_A0;
	_functions[0x21] = &UAKMScript::function_A1;
	_functions[0x22] = &UAKMScript::function_A2;
	_functions[0x23] = &UAKMScript::function_A3;
	_functions[0x24] = &UAKMScript::function_A4;
	_functions[0x25] = &UAKMScript::function_A5;
	_functions[0x26] = &UAKMScript::function_A6;
	_functions[0x27] = &UAKMScript::function_A7;
	_functions[0x28] = &UAKMScript::function_A8;
	_functions[0x29] = &UAKMScript::function_A9;
	_functions[0x2A] = &UAKMScript::function_AA;
	_functions[0x2B] = &UAKMScript::function_AB;
	_functions[0x2C] = &UAKMScript::function_AC;
	_functions[0x2D] = &UAKMScript::function_AD;
	_functions[0x2E] = &UAKMScript::function_AE;
	_functions[0x2F] = &UAKMScript::function_AF;
	_functions[0x30] = &UAKMScript::function_B0;
	_functions[0x31] = &UAKMScript::function_B1;
	_functions[0x32] = &UAKMScript::function_B2;
	_functions[0x33] = &UAKMScript::function_B3;
	_functions[0x34] = &UAKMScript::function_B4;
	_functions[0x35] = &UAKMScript::function_B5;
	_functions[0x36] = &UAKMScript::function_B6;
	_functions[0x37] = &UAKMScript::function_B7;
	_functions[0x38] = &UAKMScript::function_B8;
	_functions[0x39] = &UAKMScript::function_B9;
	_functions[0x3A] = &UAKMScript::function_BA;
	_functions[0x3B] = &UAKMScript::function_BB;
	_functions[0x3C] = &UAKMScript::function_BC;
	_functions[0x3D] = &UAKMScript::function_BD;
	_functions[0x3E] = &UAKMScript::function_BE;
	_functions[0x3F] = &UAKMScript::function_BF;
	_functions[0x40] = &UAKMScript::function_C0;
	_functions[0x41] = &UAKMScript::function_C1;
	_functions[0x42] = &UAKMScript::function_C2;
	_functions[0x43] = &UAKMScript::function_C3;
	_functions[0x44] = &UAKMScript::function_C4;
	_functions[0x45] = &UAKMScript::function_C5;
	_functions[0x46] = &UAKMScript::function_C6;
	_functions[0x47] = &UAKMScript::function_C7;
	_functions[0x48] = &UAKMScript::function_C8;
	_functions[0x49] = &UAKMScript::function_C9;
	_functions[0x4A] = &UAKMScript::function_CA;
	_functions[0x4B] = &UAKMScript::function_CB;
	_functions[0x4C] = &UAKMScript::function_CC;
	_functions[0x4D] = &UAKMScript::function_CD;
	_functions[0x4E] = &UAKMScript::function_CE;
	_functions[0x4F] = &UAKMScript::function_CF;
	_functions[0x50] = &UAKMScript::function_D0;
	_functions[0x51] = &UAKMScript::function_D1;
	_functions[0x52] = &UAKMScript::function_D2;
	_functions[0x53] = &UAKMScript::function_D3;
	_functions[0x54] = &UAKMScript::function_D4;
	_functions[0x55] = &UAKMScript::function_D5;
	_functions[0x56] = &UAKMScript::function_D6;
	_functions[0x57] = &UAKMScript::function_D7;
	_functions[0x58] = &UAKMScript::function_D8;
	_functions[0x59] = &UAKMScript::function_D9;
	_functions[0x5A] = &UAKMScript::function_DA;
	_functions[0x5B] = &UAKMScript::function_DB;
	_functions[0x5C] = &UAKMScript::function_DC;
	_functions[0x5D] = &UAKMScript::function_DD;
	_functions[0x5E] = &UAKMScript::function_DE;
	_functions[0x5F] = &UAKMScript::function_DF;
	_functions[0x60] = &UAKMScript::function_E0;
}

UAKMScript::~UAKMScript() {
}

void UAKMScript::execute(ScriptState *pState, int id) {
	if (pState == nullptr || pState->_script == nullptr)
		return;

	pState->_executionPointer = pState->getScript(id);
	pState->_waitingForMediaToFinish = false;
	pState->_lastDialoguePoint = 0;

	resume(pState);
}

void UAKMScript::resume(ScriptState *pState, bool breakWait) {
	if (pState == nullptr || pState->_script == nullptr)
		return;

	if (breakWait) {
		pState->_waitingForMediaToFinish = false;
		pState->_waitingForInput = false;
		pState->_waitingForExternalModule = false;
	}

	ModuleBase *pThisModule = ModuleController::_currentModule;

	while (ModuleController::_currentModule == pThisModule &&
		   pState->_executionPointer > -1 &&
		   pState->_executionPointer < pState->_length &&
		   !pState->_waitingForMediaToFinish &&
		   !pState->_waitingForInput &&
		   !pState->_waitingForExternalModule) {
		uint8 cmd = pState->_script[pState->_executionPointer++];
		if (cmd < 0x80 || cmd > 0xe0) {
			pState->_executionPointer = -1;
			break;
		}

		(this->*_functions[cmd - 0x80])(pState);
	}

	if (ModuleController::_currentModule != pThisModule) {
		if (!pState->_waitingForInput) {
			ModuleController::pop(pThisModule);
		}
	}
}

ActionType UAKMScript::getCurrentActions(ScriptState *pState, int currentObjectIndex) {
	if (pState == nullptr)
		return ActionType::kNone;

	pState->_queryAction = true;
	pState->_allowedAction = ActionType::kNone;
	pState->_currentAction = ActionType::kNone;
	execute(pState, currentObjectIndex);
	pState->_queryAction = false;

	return pState->_allowedAction;
}

void UAKMScript::permformAction(ScriptState *pState, int id, ActionType action, int item) {
	if (pState == nullptr)
		return;

	pState->_queryAction = false;
	pState->_currentAction = action;
	if (item > 0) {
		pState->_currentAction = ActionType::kUse;
	}
	GameController::setParameter(99, (uint8)item);

	debug(0, "UAKMScript::permformAction: ObjectIndex=%d, Action=0x%x, Item=%d", id, (int)pState->_currentAction, item);
	execute(pState, id);
}

void UAKMScript::selectDialogueOption(ScriptState *pState, int option) {
	if (pState == nullptr)
		return;

	pState->_selectedOption = option;
	resume(pState, true);
}

void UAKMScript::addCaption(ScriptState *pState, bool texTalk) {
	Common::String text;
	while (pState->_executionPointer < pState->_length) {
		char c = (char)pState->_script[pState->_executionPointer++];
		if (c == '\0')
			break;
		text += c;
	}
	Caption::add(pState->_frameTrigger, text, texTalk);
}

void UAKMScript::play(ScriptState *pState, int index, int bank, int rate) {
	Caption::swapCaptions();

	if (_mapEntry != nullptr && (_mapEntry->scriptFileEntry != 0 || _mapEntry->scriptFileIndex != 0)) {
		const Common::Array<FileMap> &map = (bank == 1) ? _mapEntry->videoMap : _mapEntry->audioMap;
		if (index >= 0 && (uint)index < map.size()) {
			FileMap fm = map[index];
			Common::String fn = GameController::getFileName(fm.file);
			debug(0, "UAKMScript::play(index=%d, bank=%d, rate=%d) -> '%s' entry %d", index, bank, rate, fn.c_str(), fm.entry);
			if (!fn.empty()) {
				AnimationController::load(fn, fm.entry, rate);
			}
		}
	}

	pState->_waitingForMediaToFinish = AnimationController::exists();
}

// 0x80: End of script block
void UAKMScript::function_80(ScriptState *pState) {
	pState->_executionPointer = -1;
}

// 0x81: Default Use fallback ("That's not gonna work.")
void UAKMScript::function_81(ScriptState *pState) {
	if (pState->_currentAction == ActionType::kUse) {
		Caption::clearAddCaptions();
		Caption::add(0, "That's not gonna work.", true);
		Caption::swapCaptions();
		AnimationController::load("SOUND.AP", 0);
		pState->_waitingForMediaToFinish = true;
	}
	pState->_executionPointer = -1;
}

// 0x82: Jump if CurrentAction == Look
void UAKMScript::function_82(ScriptState *pState) {
	int id = pState->read16();
	if (pState->_currentAction == ActionType::kLook) {
		pState->_executionPointer = pState->getScript(id);
	}
}

// 0x83: Jump if ConversationOption == Abort
void UAKMScript::function_83(ScriptState *pState) {
	int id = pState->read16();
	if (pState->_selectedOption == (int)ConversationOption::kAbort) {
		pState->_executionPointer = pState->getScript(id);
	}
}

// 0x84: Jump if CurrentAction == Get
void UAKMScript::function_84(ScriptState *pState) {
	int id = pState->read16();
	if (pState->_currentAction == ActionType::kGet) {
		pState->_executionPointer = pState->getScript(id);
	}
}

// 0x85: Jump if CurrentAction == Move
void UAKMScript::function_85(ScriptState *pState) {
	int id = pState->read16();
	if (pState->_currentAction == ActionType::kMove) {
		pState->_executionPointer = pState->getScript(id);
	}
}

// 0x86: Jump if currentAction == Use
void UAKMScript::function_86(ScriptState *pState) {
	int id = pState->read16();
	if (pState->_currentAction == ActionType::kUse) {
		pState->_executionPointer = pState->getScript(id);
	}
}

// 0x87:Jump if CurrentAction == Talk
void UAKMScript::function_87(ScriptState *pState) {
	int id = pState->read16();
	if (pState->_currentAction == ActionType::kTalk) {
		pState->_executionPointer = pState->getScript(id);
	}
}

// 0x88: Jump if CurrentAction == Open
void UAKMScript::function_88(ScriptState *pState) {
	int id = pState->read16();
	if (pState->_currentAction == ActionType::kOpen) {
		pState->_executionPointer = pState->getScript(id);
	}
}

// 0x89: Display text caption and end script
void UAKMScript::function_89(ScriptState *pState) {
	pState->_executionPointer = -1;
}

// 0x8A: Play sound + display text caption
void UAKMScript::function_8A(ScriptState *pState) {
	pState->_executionPointer = -1;
}

// 0x8B: Start mapped location Animation
void UAKMScript::function_8B(ScriptState *pState) {
	int anim = pState->read8();
	(void)anim;
	// _pLoc->startMappedAnimation(anim);
}

// 0x8C: Set Parameter[index] = value
void UAKMScript::function_8C(ScriptState *pState) {
	int index = pState->read8();
	uint8 value = (uint8)pState->read8();
	GameController::setParameter(index, value);
}

// 0x8D: Conditional jump if Parameter[ix] == cmp
void UAKMScript::function_8D(ScriptState *pState) {
	int ix = pState->read16();
	int cmp = pState->read16();
	int ns = pState->read16();
	if (GameController::getParameter(ix) == cmp) {
		pState->_executionPointer = pState->getScript(ns);
	}
}

// 0x8E: Unconditional jump to script ID
void UAKMScript::function_8E(ScriptState *pState) {
	int id = pState->read16();
	pState->_executionPointer = pState->getScript(id);
}

// 0x8F: Set player 3D position and angle
void UAKMScript::function_8F(ScriptState *pState) {
	float x = -pState->read12_4();
	float y = -pState->read12_4();
	float z = -pState->read12_4();
	float angle = -((float)pState->read16s() * 6.283185307f) / 1000.0f;
	if (_pLoc != nullptr) {
		_pLoc->setPosition(x, y, z, angle);
	}
}

// 0x90: Set ItemState(item, state)
void UAKMScript::function_90(ScriptState *pState) {
	int item = pState->read8();
	int state = pState->read8();
	GameController::setItemState(item, state);
}

// 0x91: Conditional jump if ItemState(bix) == cmp
void UAKMScript::function_91(ScriptState *pState) {
	int bix = pState->read16();
	int cmp = pState->read8();
	pState->read8(); // padding byte at +3
	int ns = pState->read16();
	if (GameController::getItemState(bix) == cmp) {
		pState->_executionPointer = pState->getScript(ns);
	}
}

// 0x92: End script
void UAKMScript::function_92(ScriptState *pState) {
	pState->_executionPointer = -1;
}

// 0x93: Load 3D Location (Map entry)
void UAKMScript::function_93(ScriptState *pState) {
	AmbientAudio::clear();
	int locationId = pState->read8();
	GameController::setParameter(252, 0);
	GameController::setWord(UAKM_SAVE_MAP_ENTRY, locationId);
	GameController::setData(UAKM_SAVE_DMAP_FLAG, (uint8)0);
	GameController::loadFromMap(locationId, GameController::getParameter(249));
	pState->_executionPointer = -1;
}

// 0x94: Load DMAP cutscene/dialogue entry
void UAKMScript::function_94(ScriptState *pState) {
	int dmapEntry = pState->read16();
	GameController::setParameter(252, 0);
	GameController::setWord(UAKM_SAVE_DMAP_ENTRY, dmapEntry);
	GameController::setData(UAKM_SAVE_DMAP_FLAG, 1);
	GameController::loadFromDMap(dmapEntry);
	pState->_waitingForInput = true;
}

// 0x95: conditional jump if animation frame == frame
void UAKMScript::function_95(ScriptState *pState) {
	int anim = pState->read8();
	pState->read8(); // padding byte
	int frame = pState->read8();
	int ns = pState->read16();
	(void)anim;
	(void)frame;
	(void)ns;
}

// 0x96: Conditional jump if animation is finished
void UAKMScript::function_96(ScriptState *pState) {
	int anim = pState->read8();
	pState->read8(); // padding byte at +1
	int ns = pState->read16();
	(void)anim;
	pState->_executionPointer = pState->getScript(ns);
}

// 0x97: Play ambient sound (1 byte)
void UAKMScript::function_97(ScriptState *pState) {
	int soundIndex = pState->read8() - 1;
	AmbientAudio::play(_mapEntry, soundIndex, true);
}

// 0x98: Conditional jump if player Y > ylimit
void UAKMScript::function_98(ScriptState *pState) {
	float ylimit = pState->read12_4();
	int ns = pState->read16();
	if (_pLoc != nullptr && _pLoc->getPlayerPosition().y() > ylimit) {
		pState->_executionPointer = pState->getScript(ns);
	}
}

// 0x99: Stop execution if querying allowed actions
void UAKMScript::function_99(ScriptState *pState) {
	if (pState->_queryAction) {
		pState->_executionPointer = -1;
	}
}

// 0x9A: Conditional jump if Player (X, Z) is inside 2D box
void UAKMScript::function_9A(ScriptState *pState) {
	float x1 = pState->read12_4();
	float x2 = pState->read12_4();
	float z1 = pState->read12_4();
	float z2 = pState->read12_4();
	int ns = pState->read16();
	if (_pLoc != nullptr) {
		Math::Vector3d pos = _pLoc->getPlayerPosition();
		float minX = MIN(x1, x2), maxX = MAX(x1, x2);
		float minZ = MIN(z1, z2), maxZ = MAX(z1, z2);
		if (pos.x() >= minX && pos.x() <= maxX && pos.z() >= minZ && pos.z() <= maxZ) {
			pState->_executionPointer = pState->getScript(ns);
		}
	}
}

// 0x9B: End script
void UAKMScript::function_9B(ScriptState *pState) {
	pState->_executionPointer = -1;
}

// 0x9C: Set Offer mode
void UAKMScript::function_9C(ScriptState *pState) {
	GameController::setParameter(253, 1);
	GameController::setParameter(100, 1);
	pState->_mode = InteractionMode::kOffer;
}

// 0x9D: Set AskAboutState(aa, state)
void UAKMScript::function_9D(ScriptState *pState) {
	int aa = pState->read8();
	uint8 state = (uint8)pState->read8();
	GameController::setAskAboutState(aa, state);
}

// 0x9E: set Timer(timer, duration)
void UAKMScript::function_9E(ScriptState *pState) {
	int timer = pState->read16();
	int duration = pState->read16();
	GameController::setTimer(timer, duration);
}

// 0x9F: Conditional jump if TimerState(timer) == state
void UAKMScript::function_9F(ScriptState *pState) {
	int timer = pState->read16();
	int state = pState->read16();
	int ns = pState->read16();
	if (GameController::getTimerState(timer) == state) {
		pState->_executionPointer = pState->getScript(ns);
	}
}

// 0xA0: Set Travel state
void UAKMScript::function_A0(ScriptState *pState) {
	int loc = pState->read8();
	uint8 val = (uint8)pState->read8();
	GameController::setData(UAKM_SAVE_TRAVEL + loc, val);
}

// 0xA1: Jump if CurrentAction == OnOff
void UAKMScript::function_A1(ScriptState *pState) {
	int id = pState->read16();
	if (pState->_currentAction == ActionType::kOnOff) {
		pState->_executionPointer = pState->getScript(id);
	}
}

void UAKMScript::function_A2(ScriptState *pState) { pState->_executionPointer = -1; }
void UAKMScript::function_A3(ScriptState *pState) { pState->_executionPointer = -1; }
void UAKMScript::function_A4(ScriptState *pState) { pState->_executionPointer = -1; }
void UAKMScript::function_A5(ScriptState *pState) {}
void UAKMScript::function_A6(ScriptState *pState) { pState->_executionPointer = -1; }

// 0xA7: Play scripted PTF video (8 bytes)
void UAKMScript::function_A7(ScriptState *pState) {
	pState->read32();
	int index = pState->read16();
	int rate = pState->read16();
	play(pState, index, 1, rate);
}

// 0xA8: Set Save Script ID (2 bytes)
void UAKMScript::function_A8(ScriptState *pState) {
	uint8 b0 = (uint8)pState->read8();
	uint8 b1 = (uint8)pState->read8();
	GameController::setData(UAKM_SAVE_SCRIPT_ID, b0);
	GameController::setData(UAKM_SAVE_SCRIPT_ID + 1, b1);
}

void UAKMScript::function_A9(ScriptState *pState) { pState->_executionPointer = -1; }
void UAKMScript::function_AA(ScriptState *pState) {}

// 0xAB: Show Picture (1 byte)
void UAKMScript::function_AB(ScriptState *pState) {
	pState->read8();
}

// 0xAC: disable travel cancel
void UAKMScript::function_AC(ScriptState *pState) {
	GameController::_canCancelTravel = false;
}

// 0xAD: Open Travel Module
void UAKMScript::function_AD(ScriptState *pState) {
	pState->_waitingForInput = true;
}

// 0xAE: Conditional jump if AskAboutState(ix) == val
void UAKMScript::function_AE(ScriptState *pState) {
	int ix = pState->read16();
	int val = pState->read16();
	int id = pState->read16();
	if (GameController::getAskAboutState(ix) == val) {
		pState->_executionPointer = pState->getScript(id);
	}
}

// 0xAF: Trigger puzzle module (6 bytes)
void UAKMScript::function_AF(ScriptState *pState) {
	pState->_executionPointer += 6;
}

void UAKMScript::function_B0(ScriptState *pState) { pState->_executionPointer = -1; }
void UAKMScript::function_B1(ScriptState *pState) { pState->_executionPointer = -1; }
void UAKMScript::function_B2(ScriptState *pState) { pState->_executionPointer = -1; }
void UAKMScript::function_B3(ScriptState *pState) { pState->_executionPointer = -1; }
void UAKMScript::function_B4(ScriptState *pState) { pState->_waitingForInput = true; }
void UAKMScript::function_B5(ScriptState *pState) { pState->_executionPointer = -1; }

// 0xB6: Allow Action (1=Look, 2=Move, 3=Open, 4=Get, 5=Talk, 6=OnOff)
void UAKMScript::function_B6(ScriptState *pState) {
	int action = pState->read16();
	switch (action) {
	case 1:
		pState->_allowedAction = static_cast<ActionType>((int)pState->_allowedAction | (int)ActionType::kLook);
		break;
	case 2:
		pState->_allowedAction = static_cast<ActionType>((int)pState->_allowedAction | (int)ActionType::kMove);
		break;
	case 3:
		pState->_allowedAction = static_cast<ActionType>((int)pState->_allowedAction | (int)ActionType::kOpen);
		break;
	case 4:
		pState->_allowedAction = static_cast<ActionType>((int)pState->_allowedAction | (int)ActionType::kGet);
		break;
	case 5:
		pState->_allowedAction = static_cast<ActionType>((int)pState->_allowedAction | (int)ActionType::kTalk);
		break;
	case 6:
		pState->_allowedAction = static_cast<ActionType>((int)pState->_allowedAction | (int)ActionType::kOnOff);
		break;
	default:
		break;
	}
}

void UAKMScript::function_B7(ScriptState *pState) { pState->_executionPointer = -1; }

// 0xB8: Conditional jump on SelectedOption == option
void UAKMScript::function_B8(ScriptState *pState) {
	int option = pState->read16();
	int ns = pState->read16();
	if (pState->_selectedOption == option) {
		pState->_executionPointer = pState->getScript(ns);
	}
}

// 0xB9: Return to LastDialoguePoint
void UAKMScript::function_B9(ScriptState *pState) {
	pState->_executionPointer = pState->_lastDialoguePoint;
}

// 0xBA: Clear AllowedAction
void UAKMScript::function_BA(ScriptState *pState) {
	pState->_allowedAction = ActionType::kNone;
}

// 0xBB: Loop ambient audio (2 bytes)
void UAKMScript::function_BB(ScriptState *pState) {
	int ix = pState->read16();
	AmbientAudio::loop(_mapEntry, ix, -1);
}

void UAKMScript::function_BC(ScriptState *pState) {}
void UAKMScript::function_BD(ScriptState *pState) { pState->_executionPointer = -1; }
void UAKMScript::function_BE(ScriptState *pState) { pState->_executionPointer = -1; }

// 0xBF: Conditional jump if ItemState(bix) != 0 (6 bytes)
void UAKMScript::function_BF(ScriptState *pState) {
	int bix = pState->read16();
	pState->read16(); // unused word
	int ns = pState->read16();
	if (GameController::getItemState(bix) != 0) {
		pState->_executionPointer = pState->getScript(ns);
	}
}

// 0xC0: Stop mapped animation (1 byte)
void UAKMScript::function_C0(ScriptState *pState) {
	pState->read8();
}

void UAKMScript::function_C1(ScriptState *pState) { pState->_executionPointer = -1; }
void UAKMScript::function_C2(ScriptState *pState) { pState->_executionPointer = -1; }
void UAKMScript::function_C3(ScriptState *pState) {}

// 0xC4: Hide Location Object (2 bytes)
void UAKMScript::function_C4(ScriptState *pState) {
	int objId = pState->read16();
	if (_pLoc != nullptr) {
		_pLoc->setObjectVisibility(objId, false);
	}
}

// 0xC5: Show Location Object (2 bytes)
void UAKMScript::function_C5(ScriptState *pState) {
	int objId = pState->read16();
	if (_pLoc != nullptr) {
		_pLoc->setObjectVisibility(objId, true);
	}
}

void UAKMScript::function_C6(ScriptState *pState) { pState->_executionPointer = -1; }
void UAKMScript::function_C7(ScriptState *pState) {}
void UAKMScript::function_C8(ScriptState *pState) { pState->_executionPointer = -1; }

// 0xC9: Add Score (2 bytes)
void UAKMScript::function_C9(ScriptState *pState) {
	int score = pState->read16s();
	GameController::addScore(score);
}

// 0xCA: Set HintCategoryState (2 bytes)
void UAKMScript::function_CA(ScriptState *pState) {
	int ix = pState->read8();
	uint8 state = (uint8)pState->read8();
	GameController::setHintCategoryState(ix, state);
}

void UAKMScript::function_CB(ScriptState *pState) {}

// 0xCC: Set HintState (2 bytes)
void UAKMScript::function_CC(ScriptState *pState) {
	pState->read16();
}

// 0xCD: Set FrameTrigger (2 bytes)
void UAKMScript::function_CD(ScriptState *pState) {
	pState->_frameTrigger = pState->read16();
}

// 0xCE: Add Caption (Tex talking)
void UAKMScript::function_CE(ScriptState *pState) {
	addCaption(pState, true);
}

// 0xCF: Add Caption (NPC talking)
void UAKMScript::function_CF(ScriptState *pState) {
	addCaption(pState, false);
}

// 0xD0: Conditional script (2 bytes)
void UAKMScript::function_D0(ScriptState *pState) {
	pState->read16();
}

// 0xD1: Play sound effect (1 byte)
void UAKMScript::function_D1(ScriptState *pState) {
	int index = pState->read8();
	play(pState, index, 0, 0);
}

void UAKMScript::function_D2(ScriptState *pState) { pState->_executionPointer = -1; }
void UAKMScript::function_D3(ScriptState *pState) { pState->_executionPointer = -1; }

// 0xD4: Stop ambient sound (1 byte)
void UAKMScript::function_D4(ScriptState *pState) {
	int index = pState->read8();
	AmbientAudio::stop(index & 0x7f);
}

// 0xD5: Load MIDI (4 bytes)
void UAKMScript::function_D5(ScriptState *pState) {
	pState->read32();
}

void UAKMScript::function_D6(ScriptState *pState) {}
void UAKMScript::function_D7(ScriptState *pState) {}
void UAKMScript::function_D8(ScriptState *pState) {}
void UAKMScript::function_D9(ScriptState *pState) {}

// 0xDA: Jump if Player distance squared to (p1, p2) <= p3 (8 bytes)
void UAKMScript::function_DA(ScriptState *pState) {
	float x = pState->read12_4();
	float z = pState->read12_4();
	float distSq = pState->read12_4();
	int ns = pState->read16();
	if (_pLoc != nullptr && _pLoc->getPlayerDistanceFromPoint(x, z) <= distSq) {
		pState->_executionPointer = pState->getScript(ns);
	}
}

// 0xDB & 0xDC: Play video (8 bytes)
void UAKMScript::function_DB(ScriptState *pState) {
	pState->read32();
	int index = pState->read16();
	int rate = pState->read16();
	play(pState, index, 1, rate);
}

void UAKMScript::function_DC(ScriptState *pState) {
	pState->read32();
	int index = pState->read16();
	int rate = pState->read16();
	play(pState, index, 1, rate);
}

void UAKMScript::function_DD(ScriptState *pState) { pState->_executionPointer = -1; }
void UAKMScript::function_DE(ScriptState *pState) { pState->_executionPointer = -1; }
void UAKMScript::function_DF(ScriptState *pState) { pState->_executionPointer = -1; }

// 0xE0: Script ID header marker (skip 2 byte script ID)
void UAKMScript::function_E0(ScriptState *pState) {
	pState->read16();
}

} // End of namespace Murphy3d
