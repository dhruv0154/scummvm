#include "amiga.h"
#include "common/debug.h"

namespace Amber {

AmigaExecutable::AmigaExecutable() {
}

AmigaExecutable::~AmigaExecutable() {
	for (uint i = 0; i < _hunks.size(); ++i) {
		if (_hunks[i].data != nullptr)
			delete[] _hunks[i].data;
	}
	_hunks.clear();
}

bool AmigaExecutable::load(Common::SeekableReadStream *stream) {
	// verify this is actually an amiga executable
	uint32 magic = stream->readUint32BE();
	if (magic != HUNK_HEADER) {
		warning("AmigaExecutable: Invalid magic number (expected 0x03F3)");
		return false;
	}

	// library strings (always 0 in ambermoon)
	uint32 libStrings = stream->readUint32BE();
	if (libStrings != 0) {
		warning("AmigaExecutable: Non-zero library strings found");
		return false;
	}

	// read the table of contents size
	uint32 numHunks = stream->readUint32BE();
	uint32 firstHunk = stream->readUint32BE();
	uint32 lastHunk = stream->readUint32BE();

	if (lastHunk - firstHunk + 1 != numHunks) {
		warning("AmigaExecutable: Hunk counts do not match");
		return false;
	}

	// read the sizes of every hunk into an array
	Common::Array<uint32> hunkSizes;
	for (uint32 i = 0; i < numHunks; ++i) {
		uint32 hunkSizeRaw = stream->readUint32BE();

		// the top 2 bits are memory flags (fast RAM or chip RAM)
		// AmigaOS extended memory flag edge case:
		if ((hunkSizeRaw & 0xC0000000) == 0xC0000000) {
			stream->readUint32BE(); // skip the extended flags
		}

		// mask out the top 2 bits and multiply by 4 (amiga sizes are in 32 bit words, not bytes)
		uint32 byteSize = (hunkSizeRaw & 0x3FFFFFFF) * 4;
		hunkSizes.push_back(byteSize);
	}

	// extract the actual blocks
	uint32 currentHunkIndex = 0;
	while (!stream->eos()) {
		uint32 typeRaw = stream->readUint32BE();
		if (stream->eos())
			break;

		HunkType type = (HunkType)(typeRaw & 0x1FFFFFFF);

		if (type == HUNK_CODE || type == HUNK_DATA) {
			uint32 numEntries = stream->readUint32BE();
			uint32 byteSize = numEntries * 4;

			Hunk hunk;
			hunk.type = type;
			hunk.size = byteSize;
			hunk.data = new byte[byteSize];
			stream->read(hunk.data, byteSize);

			_hunks.push_back(hunk);
			currentHunkIndex++;
		} else if (type == HUNK_BSS) {
			uint32 allocSize = stream->readUint32BE();
			Hunk hunk;
			hunk.type = type;
			hunk.size = allocSize * 4;
			hunk.data = nullptr; // BSS has no data in the file, it is just a memory reservation
			_hunks.push_back(hunk);
			currentHunkIndex++;
		} else if (type == HUNK_RELOC32) {
			uint32 numOffsets;
			while ((numOffsets = stream->readUint32BE()) != 0) {
				stream->readUint32BE();                 // skip hunkNumber target
				stream->seek(numOffsets * 4, SEEK_CUR); // skip the actual offsets
			}
		} else if (type == HUNK_END) {
			// just a marker saying the current block is done, we can ignore it
		} else {
			warning("AmigaExecutable: Unknown Hunk Type encountered: 0x%04X", type);
			return false;
		}
	}

	return true;
}

byte *AmigaExecutable::getDataHunk(uint targetDataIndex) const {
	uint currentDataIndex = 0;

	for (uint i = 0; i < _hunks.size(); ++i) {
		if (_hunks[i].type == HUNK_DATA) {
			if (currentDataIndex == targetDataIndex) {
				return _hunks[i].data;
			}
			currentDataIndex++;
		}
	}

	warning("AmigaExecutable: Could not find DATA hunk %d", targetDataIndex);
	return nullptr;
}

} // End of namespace Amber
