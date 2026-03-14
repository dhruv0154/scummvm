#ifndef AMBERMOON_AMIGA_EXECUTABLE_H
#define AMBERMOON_AMIGA_EXECUTABLE_H

#include "common/scummsys.h"
#include "common/stream.h"
#include "common/array.h"

namespace Amber {

enum HunkType {
	HUNK_CODE = 0x03E9,
	HUNK_DATA = 0x03EA,
	HUNK_BSS = 0x03EB,
	HUNK_RELOC32 = 0x03EC,
	HUNK_END = 0x03F2,
	HUNK_HEADER = 0x03F3
};

// container to hold each block of the executable
struct Hunk {
	HunkType type;
	uint32 size;
	byte *data;
};

class AmigaExecutable {
private:

public:
	Common::Array<Hunk> _hunks;
	AmigaExecutable();
	~AmigaExecutable();

	bool load(Common::SeekableReadStream *stream);

	byte *getDataHunk(uint targetDataIndex) const;
};

} // End of namespace Amber

#endif
