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

#include "archive.h"
#include "decoders.h"
#include "common/substream.h"
#include "common/debug.h"

namespace Amber {

AmberArchive::AmberArchive() : _containerType(0) {
	_file = new Common::File();
}

AmberArchive::~AmberArchive() {
	close();
	delete _file;
}

void AmberArchive::close() {
	_entries.clear();
	if (_file->isOpen()) {
		_file->close();
	}
}

bool AmberArchive::open(const Common::Path &filename) {
	close();

	if (!_file->open(filename)) {
		warning("AmberArchive::open - Failed to open %s", filename.toString().c_str());
		return false;
	}

	_path = filename;

	// read the 4 byte magic number ('AMNP', 'AMNC',..)
	_containerType = _file->readUint32BE();

	// verify it is a known Ambermoon container format
	if (_containerType != 0x414D4E43 && // AMNC
		_containerType != 0x414D4E50 && // AMNP
		_containerType != 0x414D4252 && // AMBR
		_containerType != 0x414D5043) { // AMPC
		warning("AmberArchive::open - Unknown container type: 0x%08X", _containerType);
		_file->close();
		return false;
	}

	// The file count is packed into 16 bit
	// top 2 bits = pointer table type
	// bottom 10 bits = number of files
	uint16 fileCountWord = _file->readUint16BE();
	int type = fileCountWord >> 14;
	uint16 fileCount = fileCountWord & 0x3FF;

	if (type == 0 || type == 2)
		return loadStandardTable(fileCount, type);
	else
		return loadSectionTable(fileCount, type);
}

bool AmberArchive::loadStandardTable(uint16 fileCount, int type) {
	// type 2 uses 16 bit sizes, type 0 uses 32 bit sizes
	int entrySize = (type == 2) ? 2 : 4;

	// data starts exactly after the 6 byte header (4 bytes magic number and 2 bytes the size of the table)
	uint32 dataOffset = 6 + (fileCount * entrySize);

	for (uint16 i = 1; i <= fileCount; i++) {
		uint32 fileSize = (type == 2) ? _file->readUint16BE() : _file->readUint32BE();

		if (fileSize > 0) {
			FileEntry entry;
			entry.id = i; // used as the JH decryption key
			entry.offset = dataOffset;
			entry.size = fileSize;

			_entries[Common::String::format("%d", i)] = entry;
		}

		// next file starts exactly where this one ends
		dataOffset += fileSize;
	}
	return true;
}

bool AmberArchive::loadSectionTable(uint16 fileCount, int type) {
	uint16 sectionCount = _file->readUint16BE(); // number of sections in this archive
	if (sectionCount == 0)
		return false;

	uint32 relOffset = 0;
	Common::HashMap<uint16, FileEntry> tempEntries; // to store files with their relative offsets

	// parse the sections (used to save space when there are empty IDs)
	for (uint16 i = 0; i < sectionCount; i++) {
		// index means at which file does this section starts
		// like let us say this section has index 1 and size 5 that means it starts from file 1
		// and goes till file 5 (file1, file2..., file5) 
		uint16 index = _file->readUint16BE();
		// setcion size is the number of files in this section
		uint16 sectionSize = _file->readUint16BE();

		for (uint16 j = 0; j < sectionSize; ++j) {
			// file size can be 32 bit or 16 bit
			uint32 fileSize = (type == 3) ? _file->readUint16BE() : _file->readUint32BE();

			FileEntry entry;
			entry.id = index;
			entry.offset = relOffset; // we store the offset relative to the start of the actual data
			entry.size = fileSize;
			tempEntries[index] = entry;

			index++;
			relOffset += fileSize;
		}
	}
	// file pos is now at the start of the actual data of files and at the end of the headers and table size
	uint32 absoluteBaseOffset = _file->pos();

	// convert all relative offsets to absolute and store the valid ones
	// since relative offset are with respect to the start of the actual data we have to add the size
	// of table and headers as well to get the actual offset from the start of the whole archive
	for (uint16 i = 1; i <= fileCount; i++) {
		if (tempEntries.contains(i) && tempEntries[i].size > 0) {
			FileEntry entry = tempEntries[i];
			entry.offset = absoluteBaseOffset + entry.offset;
			_entries[Common::String::format("%d", i)] = entry;
		}
	}
	return true;
}

bool AmberArchive::hasFile(const Common::Path &path) const {
	return _entries.contains(path.toString());
}

int AmberArchive::listMembers(Common::ArchiveMemberList &list) const {
	int count = 0;
	for (auto &it : _entries) {
		list.push_back(Common::ArchiveMemberPtr(new Common::GenericArchiveMember(it._key, *this)));
		count++;
	}
	return count;
}

const Common::ArchiveMemberPtr AmberArchive::getMember(const Common::Path &path) const {
	if (!hasFile(path))
		return nullptr;
	return Common::ArchiveMemberPtr(new Common::GenericArchiveMember(path, *this));
}

Common::SeekableReadStream *AmberArchive::createReadStreamForMember(const Common::Path &path) const {
	Common::String filename = path.toString();
	if (!_entries.contains(filename))
		return nullptr;

	FileEntry entry = _entries[filename];
	Common::SeekableReadStream *rawStream = new Common::SeekableSubReadStream(_file, entry.offset, entry.offset + entry.size);

	if (!rawStream)
		return nullptr;

	// AMNC files are 100% encrypted, we cannot read a single
	// byte (not even the LOB header) until we decrypt the entire data
	if (_containerType == 0x414D4E43) {
		// decrypt the entire data, using file id as decryption key
		Common::SeekableReadStream *decrypted = createJHStream(rawStream, entry.id, entry.size);

		// the rawStream is no longer needed since we now have the decrypted bytes loaded
		delete rawStream;
		if (!decrypted)
			return nullptr;

		// now that it is decrypted, we can check if it is LOB compressed
		uint32 magic = decrypted->readUint32BE();
		if (magic == 0x014C4F42) {
			uint32 lobHeader = decrypted->readUint32BE();
			uint32 decodedSize = lobHeader & 0x00FFFFFF; // extract bottom 24 bits for size

			// a normal LOB header has 4 more bytes for compressedSize, we don not need it, so we skip it
			decrypted->readUint32BE();

			// stream is now positioned exactly at the readable actual data
			Common::SeekableReadStream *decompressed = createLOBStream(decrypted, decodedSize);

			delete decrypted;
			return decompressed; // return the final, clean, uncompressed data
		}

		// if the magic number was not LOB, it was just an uncompressed encrypted file
		decrypted->seek(0);
		return decrypted;
	}

	// AMNP files are built for speed, to let the engine pre-allocate memory
	// instantly, the original devs left the first 8 bytes (LOB magic number + decoded size) unencrypted
	// but the rest of the file (including the LOB compressedSize) is JH encrypted
	if (_containerType == 0x414D4E50) {
		// read the raw, unencrypted bytes directly
		uint32 magic = rawStream->readUint32BE();

		if (magic == 0x014C4F42) {
			// bytes 4 to 7 are also unencrypted plain text
			uint32 lobHeader = rawStream->readUint32BE();
			uint32 decodedSize = lobHeader & 0x00FFFFFF;

			// decrypt the rest of the data
			// because we already read 8 bytes, the rawStream cursor is at byte 8
			// we tell the function to only process the remaining (size - 8) bytes
			Common::SeekableReadStream *decrypted = createJHStream(rawStream, entry.id, entry.size - 8);
			delete rawStream;
			if (!decrypted)
				return nullptr;

			// the first 4 bytes of our decrypted data is actually the compressedSize
			// we must skip it to reach the actual data
			decrypted->readUint32BE();

			// decompress the LOB files since they are decrypted now
			Common::SeekableReadStream *decompressed = createLOBStream(decrypted, decodedSize);
			delete decrypted;
			return decompressed;

		} else {
			// fallback for if an AMNP file does not start with LOB then we decrypt the whole thing
			rawStream->seek(0);
			Common::SeekableReadStream *decrypted = createJHStream(rawStream, entry.id, entry.size);
			delete rawStream;
			return decrypted;
		}
	}

	// AMPC files are purely LOB compressed 
	if (_containerType == 0x414D5043) {
		uint32 magic = rawStream->readUint32BE();

		if (magic == 0x014C4F42) {
			uint32 lobHeader = rawStream->readUint32BE();
			uint32 decodedSize = lobHeader & 0x00FFFFFF;

			// skip the 4 bytes for compressed size
			rawStream->readUint32BE();

			Common::SeekableReadStream *decompressed = createLOBStream(rawStream, decodedSize);
			delete rawStream;
			return decompressed;
		}
		rawStream->seek(0);
	}

	// AMBR files have no encryption at all
	return rawStream;
}

} // End of namespace Amber
