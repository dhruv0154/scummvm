#include "murphy3d/archive.h"
#include "common/substream.h"
#include "common/memstream.h"
#include "common/compression/access_lzw.h"

namespace Murphy3d {

Archive::Archive() {
}

Archive::~Archive() {
	close();
}

bool Archive::open(const Common::Path &filename) {
	close();
	
	if (!_file.open(filename))
		return false;

	uint16 count = _file.readUint16LE();
	_fileIndex.resize(count);

	for (uint16 i = 0; i < count; i++) {
		_fileIndex[i] = _file.readUint32LE();
	}

	return true;
}

void Archive::close() {
	_file.close();
	_fileIndex.clear();
}

Common::SeekableReadStream* Archive::getStream(uint subfile) {
	if (subfile >= _fileIndex.size())
		return nullptr;

	uint32 offset = _fileIndex[subfile];
	uint32 size = (subfile == _fileIndex.size() - 1) ? _file.size() - offset : _fileIndex[subfile + 1] - offset;
	_file.seek(offset);

	char magic[3];
	_file.read(magic, 3);

	if (strncmp(magic, "DBE", 3) == 0) {
		_file.seek(offset);

		byte *compressedData = new byte[size];
		_file.read(compressedData, size);

		byte *decompressedData = nullptr;

		uint32 decompressedSize = Common::decompressAccessDBE(compressedData, &decompressedData);
		delete[] compressedData;

		return new Common::MemoryReadStream(decompressedData, decompressedSize, DisposeAfterUse::YES);
	} else {
		return new Common::SeekableSubReadStream(&_file, offset, offset + size);
	}
}

} // namespace Murphy3d
