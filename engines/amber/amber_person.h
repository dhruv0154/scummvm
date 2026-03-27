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

#ifndef AMBER_PERSON_H
#define AMBER_PERSON_H

#include "common/scummsys.h"
#include "common/str.h"

namespace Amber {

class AmberPerson {
public:
	Common::String _name;
	uint16 _portraitId;
	uint16 _currentHP;
	uint16 _maxHP;
	uint16 _currentSP;
	uint16 _maxSP;
	bool _hasMagic;
	uint8 _characterClass;

	AmberPerson(const Common::String &name, uint16 portraitId, uint16 maxHp, uint16 maxSp, bool hasMagic)
		: _name(name), _portraitId(portraitId), _currentHP(maxHp), _maxHP(maxHp),
		  _currentSP(maxSp), _maxSP(maxSp), _hasMagic(hasMagic), _characterClass(0) {}

	virtual ~AmberPerson() {}

	virtual void levelUp() = 0;
	virtual bool tryJoinParty() = 0;
};

} // End of namespace Amber
#endif // AMBER_PERSON_H
