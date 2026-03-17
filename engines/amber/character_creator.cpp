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

#include "character_creator.h"
#include "amber/amber.h"
#include "amber/archive.h" 
#include "amber/decoders.h"
#include "graphics/framelimiter.h"
#include "common/system.h"


namespace Amber {

// these map directly to the file names inside "Portraits.amb" (e.g., file "2" is the first male)
const int CharacterCreator::_malePortraitIds[4] = {2, 25, 7, 23};
const int CharacterCreator::_femalePortraitIds[4] = {31, 38, 44, 51};

CharacterCreator::CharacterCreator(AmberEngine *engine) : _engine(engine) {
	// initialize default state
	_playerName = "THALION";
	_pressedButtonId = -1; // -1 means the mouse is not pressing any button
	_isFemale = false;
	_portraitListIndex = 0;
	_iconMale = _iconFemale = _iconLeft = _iconRight = _iconOk = nullptr;

	// we calculate the center of the 320x200 screen once here
	_winX = (320 - (16 * 16)) / 2;
	_winY = (200 - (6 * 16)) / 2 - 8;

	// we pre-calculate the math for the buttons here so the event loop
	// just does a simple rect.contains(mousePos) check
	_buttons[0] = {0, Common::Rect(_winX + 15, _winY + 25, _winX + 49, _winY + 44)};   // Male
	_buttons[1] = {1, Common::Rect(_winX + 15, _winY + 44, _winX + 49, _winY + 63)};   // Female
	_buttons[2] = {2, Common::Rect(_winX + 71, _winY + 34, _winX + 105, _winY + 53)};  // Left
	_buttons[3] = {3, Common::Rect(_winX + 151, _winY + 34, _winX + 185, _winY + 53)}; // Right
	_buttons[4] = {4, Common::Rect(_winX + 207, _winY + 62, _winX + 241, _winY + 81)}; // OK
}

CharacterCreator::~CharacterCreator() {
	if (_iconMale) {
		_iconMale->free();
		delete _iconMale;
	}
	if (_iconFemale) {
		_iconFemale->free();
		delete _iconFemale;
	}
	if (_iconLeft) {
		_iconLeft->free();
		delete _iconLeft;
	}
	if (_iconRight) {
		_iconRight->free();
		delete _iconRight;
	}
	if (_iconOk) {
		_iconOk->free();
		delete _iconOk;
	}
}

void CharacterCreator::loadAssets() {
	Common::File btnFile;
	if (btnFile.open("Button_graphics")) {
		Common::SeekableReadStream *activeStream = &btnFile;
		Common::SeekableReadStream *decryptedStream = nullptr;
		Common::SeekableReadStream *decompressedStream = nullptr;

		// decrypt (JH)
		uint32 header = activeStream->readUint32BE();
		if ((header & 0xFFFF0000) == 0x4A480000) {
			uint16 key = ((header >> 16) & 0xFFFF) ^ (header & 0xFFFF);
			decryptedStream = createJHStream(activeStream, key, btnFile.size() - 4);
			activeStream = decryptedStream;
			header = activeStream->readUint32BE(); // read the next header from the decrypted data
		}

		// decompress (LOB)
		if (header == 0x014C4F42) {
			uint32 decodedSize = activeStream->readUint32BE() & 0x00FFFFFF;
			activeStream->readUint32BE();
			decompressedStream = createLOBStream(activeStream, decodedSize);
			activeStream = decompressedStream;
		} else {
			// if it was not LOB compressed, we need to rewind the stream back 4 bytes
			// because we consumed the header but did not use it
			activeStream->seek(0);
		}

		// each button sprite is exactly 156 bytes long
		// 32 pixels wide, 13 pixels tall and 3 bit color, each plane is 52 bytes long
		activeStream->seek(76 * 156);
		_iconMale = _engine->decodePlanarGraphic(activeStream, 32, 13, 3, 24);

		activeStream->seek(77 * 156);
		_iconFemale = _engine->decodePlanarGraphic(activeStream, 32, 13, 3, 24);

		activeStream->seek(11 * 156);
		_iconLeft = _engine->decodePlanarGraphic(activeStream, 32, 13, 3, 24);

		activeStream->seek(12 * 156);
		_iconRight = _engine->decodePlanarGraphic(activeStream, 32, 13, 3, 24);

		activeStream->seek(28 * 156);
		_iconOk = _engine->decodePlanarGraphic(activeStream, 32, 13, 3, 24);

		if (decompressedStream)
			delete decompressedStream;
		if (decryptedStream)
			delete decryptedStream;
		btnFile.close();
	}
}

void CharacterCreator::drawPortrait() {
	AmberArchive portraitArchive;
	if (portraitArchive.open(Common::Path("Portraits.amb"))) {

		// figure out which ID to load based on our current state
		int portraitId = _isFemale ? _femalePortraitIds[_portraitListIndex] : _malePortraitIds[_portraitListIndex];
		Common::String idStr = Common::String::format("%d", portraitId);

		Common::SeekableReadStream *portraitStream = portraitArchive.createReadStreamForMember(Common::Path(idStr));

		if (portraitStream) {
			Graphics::Surface *portraitSurf = _engine->decodePlanarGraphic(portraitStream, 32, 34, 5, 0);
			if (portraitSurf) {
				int winX = (320 - (16 * 16)) / 2;
				int winY = (200 - (6 * 16)) / 2 - 8;
				_engine->_screen->copyRectToSurface(*portraitSurf, winX + 112, winY + 32, Common::Rect(0, 0, 32, 34));

				portraitSurf->free();
				delete portraitSurf;
			}
			delete portraitStream;
		}
		portraitArchive.close();
	}
}

void CharacterCreator::handleEvent(const Common::Event &e, const Common::Point &mousePos) {
	// ambermoon limits names to 13 characters so they perfectly fit
	// inside the 16x1 sunken text box
	const uint maxNameLen = 13;

	switch (e.type) {

	case Common::EVENT_LBUTTONDOWN:
		// loop through our 5 hitboxes
		// if the mouse clicked inside one, we register that button as currently being pressed
		for (int i = 0; i < 5; i++) {
			if (_buttons[i].rect.contains(mousePos)) {
				_pressedButtonId = _buttons[i].id;
				break;
			}
		}
		break;

	case Common::EVENT_MOUSEMOVE:
		// if the user clicked a button, but changed their mind and dragged the mouse away,
		// we must cancel the click.
		if (_pressedButtonId != -1 && !_buttons[_pressedButtonId].rect.contains(mousePos))
			_pressedButtonId = -1; // reset to idle state
		break;

	case Common::EVENT_LBUTTONUP:
		// we only trigger the action if the mouse is released inside the button that was originally pressed
		if (_pressedButtonId != -1 && _buttons[_pressedButtonId].rect.contains(mousePos)) {
			switch (_pressedButtonId) {
			case 0: // male button
				if (_isFemale) {
					_isFemale = false;
					_portraitListIndex = 0;
				}
				break;
			case 1: // female button
				if (!_isFemale) {
					_isFemale = true;
					_portraitListIndex = 0;
				}
				break;
			case 2: // left arrow
				// wrap around the array backwards, if we go below 0, jump to index 3
				_portraitListIndex = (_portraitListIndex - 1 < 0) ? 3 : _portraitListIndex - 1;
				break;
			case 3: // right arrow
				// wrap around the array forwards, if we go above 3, jump back to index 0
				_portraitListIndex = (_portraitListIndex + 1 > 3) ? 0 : _portraitListIndex + 1;
				break;
			case 4: // OK button
				// this logs the final output, in the future, this is where we will set a flag
				// to exit this loop and start the game
				warning("Character Created: Name=%s, Female=%d, PortraitIndex=%d",
						_playerName.c_str(), _isFemale, _portraitListIndex);
				break;
			}
		}
		_pressedButtonId = -1;
		break;

	case Common::EVENT_KEYDOWN:
		if (e.kbd.keycode == Common::KEYCODE_BACKSPACE) {
			if (_playerName.size() > 0) {
				_playerName.deleteChar(_playerName.size() - 1); // delete the last character
			}
		}
		else if (_playerName.size() < maxNameLen) {
			char c = (char)e.kbd.ascii;

			// only allow standard alphabet, numbers, and spaces
			if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ') {

				// ambermoon forced uppercase for character names
				if (c >= 'a' && c <= 'z')
					c -= 32;

				_playerName += c;
			}
		}
		break;

	default:
		break;
	}
}


void CharacterCreator::draw() {
	Graphics::Screen *screen = _engine->_screen;

	// draw the background window and text input box
	_engine->_ui->drawWindow(screen, _winX, _winY, 16, 6);
	_engine->_ui->drawBox(screen, Common::Rect(_winX + 68, _winY + 68, _winX + 180, _winY + 82), true);

	// draw the interactive buttons borders
	for (int i = 0; i < 5; i++) {
		bool isPressed = (_pressedButtonId == i);
		_engine->_ui->drawBox(screen, _buttons[i].rect, true);

		// draw the actual raised stone button, if isPressed is true it will draw a depressed button
		_engine->_ui->drawButton(screen, _buttons[i].rect.left + 1, _buttons[i].rect.top + 1, isPressed);
	}

	// draw the button icons
	// ambermoon adds a +2 pixel y offset to the icon when the button is pressed
	// this makes the icon physically sink into the screen

	int yOffset = 0;
	if (_iconMale) {
		yOffset = (_pressedButtonId == 0) ? 2 : 0;
		screen->transBlitFrom(*_iconMale, Common::Point(_winX + 16, _winY + 28 + yOffset), 24);
	}
	if (_iconFemale) {
		yOffset = (_pressedButtonId == 1) ? 2 : 0;
		screen->transBlitFrom(*_iconFemale, Common::Point(_winX + 16, _winY + 47 + yOffset), 24);
	}
	if (_iconLeft) {
		yOffset = (_pressedButtonId == 2) ? 2 : 0;
		screen->transBlitFrom(*_iconLeft, Common::Point(_winX + 72, _winY + 37 + yOffset), 24);
	}
	if (_iconRight) {
		yOffset = (_pressedButtonId == 3) ? 2 : 0;
		screen->transBlitFrom(*_iconRight, Common::Point(_winX + 152, _winY + 37 + yOffset), 24);
	}
	if (_iconOk) {
		yOffset = (_pressedButtonId == 4) ? 2 : 0;
		screen->transBlitFrom(*_iconOk, Common::Point(_winX + 208, _winY + 65 + yOffset), 24);
	}

	_engine->_font->drawString(screen, "CREATE YOUR CHARACTER :", _winX + 44, _winY + 16);
	_engine->_font->drawString(screen, _playerName, _winX + 74, _winY + 71);

	// blinking cursor
	if ((g_system->getMillis() / 500) % 2 == 0) {
		// start at the text X origin (74), and add 6 pixels for every character typed
		int cursorX = _winX + 74 + (_playerName.size() * 6);

		// draw a 6x6 pixel solid rectangle, color 31 is for white
		screen->fillRect(Common::Rect(cursorX, _winY + 71, cursorX + 6, _winY + 77), 31);
	}

	drawPortrait();
}

void CharacterCreator::execute() {
	loadAssets();
	Common::Event e;
	Graphics::FrameLimiter limiter(g_system, 60);

	while (!_engine->shouldQuit()) {
		Common::Point mousePos = g_system->getEventManager()->getMousePos();
		while (g_system->getEventManager()->pollEvent(e))
			handleEvent(e, mousePos);

		draw();
		limiter.delayBeforeSwap();
		_engine->_screen->update();
		limiter.startFrame();
	}
}

} // End of namespace Amber
