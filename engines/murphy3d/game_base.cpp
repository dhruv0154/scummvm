#include "murphy3d/game_base.h"
#include "common/endian.h"
#include "common/str.h"
#include "common/file.h"
#include "common/memstream.h"
#include "graphics/surface.h"
#include "murphy3d/archive.h"
#include "murphy3d/game_controller.h"
#include "murphy3d/location_module.h"
#include "murphy3d/module_controller.h"
#include "murphy3d/video_module.h"

namespace Murphy3d {

GameBase::GameBase() {
	_gameData = nullptr;
	memset(_timers, 0, sizeof(_timers));
	_hintCategoryCount = 0;
	_allowCancelVideo = true;
	_selectedItem = 0;
}

GameBase::~GameBase() {
	for (int i = 0; i < 13; i++) {
		ModuleController::_cursors[i].dispose();
	}
	for (auto &it : _hintCategories) {
		delete it._value;
	}
	_hintCategories.clear();
}

void GameBase::loadFromDMap(int entry) {
	ModuleController::push(new VideoModule(VideoType::kScripted, entry));
}

void GameBase::loadFromMap(int entry, int startupPosition) {
	ModuleController::push(getLocationModule(entry, startupPosition));
}

LocationModule *GameBase::getLocationModule(int entry, int startupPosition) {
	return new LocationModule(entry, startupPosition);
}

bool GameBase::loadIcons(const byte *bdData, uint32 bdLength) {
	if (bdData == nullptr || bdLength < 2)
		return false;

	int count = (int)READ_LE_INT16(bdData);

	uint8 colourTranslationTable[64];
	for (int i = 0; i < 64; i++) {
		colourTranslationTable[i] = static_cast<uint8>(4.04762 * i);
	}

	Archive gfxArchive;
	if (!gfxArchive.open("GRAPHICS.AP"))
		return false;

	Common::SeekableReadStream *palStream = gfxArchive.getStream(0);
	if (!palStream || palStream->size() < 768) {
		delete palStream;
		return false;
	}

	byte paletteData[768];
	palStream->read(paletteData, 768);
	delete palStream;

	for (int i = 0; i < (count - 2) && i < 12; i++) {
		uint32 iconOffset = READ_LE_UINT32(bdData + 2 + i * 4);
		uint32 nextOffset = READ_LE_UINT32(bdData + 6 + i * 4);
		if (iconOffset >= bdLength || nextOffset > bdLength || nextOffset <= iconOffset)
			continue;

		const uint8 *pIcon = bdData + iconOffset;
		int size = static_cast<int>(nextOffset - iconOffset);
		const uint8 *pEnd = pIcon + size;

		if (size > 21) {
			memcpy(paletteData + 3, pIcon, 21);
			const uint8 *pPalette = paletteData;
			pIcon += 21;

			Common::Array<Graphics::Surface *> icons;
			while (pIcon + 16 <= pEnd) {
				int w = READ_LE_UINT16(pIcon + 2);
				int h = READ_LE_UINT16(pIcon + 4);
				int dataSize = READ_LE_UINT32(pIcon + 9);
				if (dataSize <= 0 || pIcon + dataSize > pEnd)
					break;

				Graphics::Surface *surf = new Graphics::Surface();
				surf->create(w, h, Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0));
				memset(surf->getPixels(), 0, w * h * 4);

				const uint8 *pSrc = pIcon + 16;
				for (int y = 0; y < h; y++) {
					int o = READ_LE_UINT16(pSrc + 0);
					int l = READ_LE_UINT16(pSrc + 2);

					uint8 *pDst = static_cast<uint8 *>(surf->getBasePtr(0, y));
					for (int x = 0; x < l; x++) {
						int c = pSrc[4 + x];
						if (c > 0 && (o + x) < w) {
							pDst[(o + x) * 4 + 3] = colourTranslationTable[pPalette[c * 3 + 0] & 0x3f];
							pDst[(o + x) * 4 + 2] = colourTranslationTable[pPalette[c * 3 + 1] & 0x3f];
							pDst[(o + x) * 4 + 1] = colourTranslationTable[pPalette[c * 3 + 2] & 0x3f];
							pDst[(o + x) * 4 + 0] = 0xff;
						}
					}
					pSrc += 4 + l;
				}

				icons.push_back(surf);
				pIcon += dataSize;
			}

			ModuleController::_cursors[i].setIcons(static_cast<CursorType>(i), icons);

			for (uint k = 0; k < icons.size(); k++) {
				icons[k]->free();
				delete icons[k];
			}
		}
	}

	return true;
}

void GameBase::readGameXMLInfo(const Common::String &xmlFilename) {
	Common::File f;
	if (!f.open(Common::Path(xmlFilename)))
		return;

	uint32 xmlSize = f.size();
	if (xmlSize == 0)
		return;

	char *buf = new char[xmlSize + 1];
	f.read(buf, xmlSize);
	buf[xmlSize] = '\0';
	Common::String sXml(buf);
	delete[] buf;

	auto getAttr = [](const Common::String &tag, const Common::String &attr) -> Common::String {
		Common::String needle = attr + "=\"";
		size_t start = tag.find(needle);
		if (start == Common::String::npos)
			return "";
		start += needle.size();
		size_t end = tag.find("\"", start);
		if (end == Common::String::npos)
			return "";
		return tag.substr(start, end - start);
	};

	// Files
	{
		size_t start = sXml.find("<Files>");
		size_t end = sXml.find("</Files>");
		if (start != Common::String::npos && end != Common::String::npos) {
			Common::String section = sXml.substr(start, end - start);
			size_t pos = 0;
			int ix = 0;
			while ((pos = section.find("<File ", pos)) != Common::String::npos) {
				size_t tagEnd = section.find("/>", pos);
				if (tagEnd == Common::String::npos)
					break;
				Common::String tag = section.substr(pos, tagEnd - pos);
				Common::String val = getAttr(tag, "Name");
				if (!val.empty())
					GameController::setFileName(ix++, val);
				pos = tagEnd;
			}
		}
	}

	// AskAbout
	{
		size_t start = sXml.find("<AskAbout>");
		size_t end = sXml.find("</AskAbout>");
		if (start != Common::String::npos && end != Common::String::npos) {
			Common::String section = sXml.substr(start, end - start);
			size_t pos = 0;
			int ix = 0;
			while ((pos = section.find("<Topic ", pos)) != Common::String::npos) {
				size_t tagEnd = section.find("/>", pos);
				if (tagEnd == Common::String::npos)
					break;
				Common::String tag = section.substr(pos, tagEnd - pos);
				Common::String val = getAttr(tag, "Name");
				if (!val.empty())
					GameController::setAskAboutName(ix++, val);
				pos = tagEnd;
			}
		}
	}

	// Items
	{
		size_t start = sXml.find("<Items>");
		size_t end = sXml.find("</Items>");
		if (start != Common::String::npos && end != Common::String::npos) {
			Common::String section = sXml.substr(start, end - start);
			size_t pos = 0;
			int ix = 0;
			while ((pos = section.find("<Item ", pos)) != Common::String::npos) {
				size_t tagEnd = section.find("/>", pos);
				if (tagEnd == Common::String::npos)
					break;
				Common::String tag = section.substr(pos, tagEnd - pos);
				Common::String val = getAttr(tag, "Name");
				if (!val.empty())
					GameController::setItemName(ix++, val);
				pos = tagEnd;
			}
		}
	}

	// SaveSituations
	{
		size_t start = sXml.find("<SaveSituations>");
		if (start != Common::String::npos) {
			size_t lStart = sXml.find("<L ", start);
			size_t lEnd = sXml.find("</L>", lStart);
			if (lStart != Common::String::npos && lEnd != Common::String::npos) {
				Common::String lSec = sXml.substr(lStart, lEnd - lStart);
				size_t defPos = lSec.find("Default=\"");
				if (defPos != Common::String::npos) {
					size_t qEnd = lSec.find("\"", defPos + 9);
					Common::String defVal = lSec.substr(defPos + 9, qEnd - (defPos + 9));
					GameController::setSituationDescriptionL(-1, defVal);
				}
				size_t pos = 0;
				int ix = 0;
				while ((pos = lSec.find("<LD ", pos)) != Common::String::npos) {
					size_t tagEnd = lSec.find("/>", pos);
					Common::String tag = lSec.substr(pos, tagEnd - pos);
					Common::String val = getAttr(tag, "Name");
					if (!val.empty())
						GameController::setSituationDescriptionL(ix++, val);
					pos = tagEnd;
				}
			}

			size_t dStart = sXml.find("<D ", start);
			size_t dEnd = sXml.find("</D>", dStart);
			if (dStart != Common::String::npos && dEnd != Common::String::npos) {
				Common::String dSec = sXml.substr(dStart, dEnd - dStart);
				size_t defPos = dSec.find("Default=\"");
				if (defPos != Common::String::npos) {
					size_t qEnd = dSec.find("\"", defPos + 9);
					Common::String defVal = dSec.substr(defPos + 9, qEnd - (defPos + 9));
					GameController::setSituationDescriptionD(-1, defVal);
				}
				size_t pos = 0;
				int ix = 0;
				while ((pos = dSec.find("<DD ", pos)) != Common::String::npos) {
					size_t tagEnd = dSec.find("/>", pos);
					Common::String tag = dSec.substr(pos, tagEnd - pos);
					Common::String val = getAttr(tag, "Name");
					if (!val.empty())
						GameController::setSituationDescriptionD(ix++, val);
					pos = tagEnd;
				}
			}
		}
	}

	// Hints
	{
		size_t start = sXml.find("<Hints>");
		size_t end = sXml.find("</Hints>");
		if (start != Common::String::npos && end != Common::String::npos) {
			Common::String section = sXml.substr(start, end - start);
			size_t pos = 0;
			_hintCategoryCount = 0;
			while ((pos = section.find("<HintCategory ", pos)) != Common::String::npos) {
				size_t startTagEnd = section.find(">", pos);
				size_t endTag = section.find("</HintCategory>", startTagEnd);
				Common::String catTag = section.substr(pos, startTagEnd - pos);
				Common::String catContent = section.substr(startTagEnd + 1, endTag - (startTagEnd + 1));

				Common::String idxStr = getAttr(catTag, "Index");
				Common::String title = getAttr(catTag, "Title");
				int catIdx = atoi(idxStr.c_str());
				if (_hintCategoryCount < catIdx)
					_hintCategoryCount = catIdx;

				HintCategory *pCategory = new HintCategory(catIdx, title);

				size_t hPos = 0;
				while ((hPos = catContent.find("<Hint ", hPos)) != Common::String::npos) {
					size_t hStartTagEnd = catContent.find(">", hPos);
					size_t hEndTag = catContent.find("</Hint>", hStartTagEnd);
					Common::String hTag = catContent.substr(hPos, hStartTagEnd - hPos);
					Common::String hText = catContent.substr(hStartTagEnd + 1, hEndTag - (hStartTagEnd + 1));

					Common::String hIdxStr = getAttr(hTag, "Index");
					int hIdx = atoi(hIdxStr.c_str());

					pCategory->addHint(hIdx, hText);
					hPos = hEndTag;
				}

				_hintCategories[catIdx] = pCategory;
				pos = endTag;
			}
		}
	}
}

void GameBase::start() {
	init();
	if (Common::File::exists("TITLE.AP")) {
		ModuleController::push(new VideoModule(VideoType::kSingle, "TITLE.AP", 0));
	}
}

HintCategory *GameBase::getHintCategory(int index) {
	if (_hintCategories.contains(index)) {
		return _hintCategories[index];
	}
	return nullptr;
}

} // End of namespace Murphy3d
