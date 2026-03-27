/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef AMBERMOON_PERSON_H
#define AMBERMOON_PERSON_H

#include "amber/amber_person.h"

namespace Amber {

class AmbermoonPerson : public AmberPerson {
public:
	uint16 _activeSpellDurations[6];
	uint8 _chestLockedStates[32];

	AmbermoonPerson(const Common::String &name, uint16 portraitId, uint16 maxHp, uint16 maxSp, bool hasMagic)
		: AmberPerson(name, portraitId, maxHp, maxSp, hasMagic) {
		for (int i = 0; i < 6; i++)
			_activeSpellDurations[i] = 0;
		for (int i = 0; i < 32; i++)
			_chestLockedStates[i] = 0;
	}

	void levelUp() override {
		// TODO: implement ambermoon specific logic
	}

	bool tryJoinParty() override {
		// TODO: implement ambermoon specific logic
		return true;
	}
};

} // End of namespace Amber
#endif // AMBERMOON_PERSON_H
