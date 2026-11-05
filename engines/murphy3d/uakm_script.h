#ifndef MURPHY3D_UAKM_SCRIPT_H
#define MURPHY3D_UAKM_SCRIPT_H

#include "murphy3d/script_base.h"

namespace Murphy3d {

class UAKMScript : public ScriptBase {
public:
	UAKMScript();
	~UAKMScript() override;

	void execute(ScriptState *pState, int id) override;
	void resume(ScriptState *pState, bool breakWait = false) override;
	ActionType getCurrentActions(ScriptState *pState, int currentObjectIndex) override;
	void permformAction(ScriptState *pState, int id, ActionType action, int item) override;
	void selectDialogueOption(ScriptState *pState, int option) override;

protected:
	void (UAKMScript::*_functions[97])(ScriptState *pState);

	void function_80(ScriptState *pState);
	void function_81(ScriptState *pState);
	void function_82(ScriptState *pState);
	void function_83(ScriptState *pState);
	void function_84(ScriptState *pState);
	void function_85(ScriptState *pState);
	void function_86(ScriptState *pState);
	void function_87(ScriptState *pState);
	void function_88(ScriptState *pState);
	void function_89(ScriptState *pState);
	void function_8A(ScriptState *pState);
	void function_8B(ScriptState *pState);
	void function_8C(ScriptState *pState);
	void function_8D(ScriptState *pState);
	void function_8E(ScriptState *pState);
	void function_8F(ScriptState *pState);
	void function_90(ScriptState *pState);
	void function_91(ScriptState *pState);
	void function_92(ScriptState *pState);
	void function_93(ScriptState *pState);
	void function_94(ScriptState *pState);
	void function_95(ScriptState *pState);
	void function_96(ScriptState *pState);
	void function_97(ScriptState *pState);
	void function_98(ScriptState *pState);
	void function_99(ScriptState *pState);
	void function_9A(ScriptState *pState);
	void function_9B(ScriptState *pState);
	void function_9C(ScriptState *pState);
	void function_9D(ScriptState *pState);
	void function_9E(ScriptState *pState);
	void function_9F(ScriptState *pState);
	void function_A0(ScriptState *pState);
	void function_A1(ScriptState *pState);
	void function_A2(ScriptState *pState);
	void function_A3(ScriptState *pState);
	void function_A4(ScriptState *pState);
	void function_A5(ScriptState *pState);
	void function_A6(ScriptState *pState);
	void function_A7(ScriptState *pState);
	void function_A8(ScriptState *pState);
	void function_A9(ScriptState *pState);
	void function_AA(ScriptState *pState);
	void function_AB(ScriptState *pState);
	void function_AC(ScriptState *pState);
	void function_AD(ScriptState *pState);
	void function_AE(ScriptState *pState);
	void function_AF(ScriptState *pState);
	void function_B0(ScriptState *pState);
	void function_B1(ScriptState *pState);
	void function_B2(ScriptState *pState);
	void function_B3(ScriptState *pState);
	void function_B4(ScriptState *pState);
	void function_B5(ScriptState *pState);
	void function_B6(ScriptState *pState);
	void function_B7(ScriptState *pState);
	void function_B8(ScriptState *pState);
	void function_B9(ScriptState *pState);
	void function_BA(ScriptState *pState);
	void function_BB(ScriptState *pState);
	void function_BC(ScriptState *pState);
	void function_BD(ScriptState *pState);
	void function_BE(ScriptState *pState);
	void function_BF(ScriptState *pState);
	void function_C0(ScriptState *pState);
	void function_C1(ScriptState *pState);
	void function_C2(ScriptState *pState);
	void function_C3(ScriptState *pState);
	void function_C4(ScriptState *pState);
	void function_C5(ScriptState *pState);
	void function_C6(ScriptState *pState);
	void function_C7(ScriptState *pState);
	void function_C8(ScriptState *pState);
	void function_C9(ScriptState *pState);
	void function_CA(ScriptState *pState);
	void function_CB(ScriptState *pState);
	void function_CC(ScriptState *pState);
	void function_CD(ScriptState *pState);
	void function_CE(ScriptState *pState);
	void function_CF(ScriptState *pState);
	void function_D0(ScriptState *pState);
	void function_D1(ScriptState *pState);
	void function_D2(ScriptState *pState);
	void function_D3(ScriptState *pState);
	void function_D4(ScriptState *pState);
	void function_D5(ScriptState *pState);
	void function_D6(ScriptState *pState);
	void function_D7(ScriptState *pState);
	void function_D8(ScriptState *pState);
	void function_D9(ScriptState *pState);
	void function_DA(ScriptState *pState);
	void function_DB(ScriptState *pState);
	void function_DC(ScriptState *pState);
	void function_DD(ScriptState *pState);
	void function_DE(ScriptState *pState);
	void function_DF(ScriptState *pState);
	void function_E0(ScriptState *pState);

	void addCaption(ScriptState *pState, bool texTalk);
	void play(ScriptState *pState, int index, int bank, int rate);
	void playAudio(ScriptState *pState, int index);
};

} // End of namespace Murphy3d

#endif
