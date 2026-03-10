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

#ifndef AMBER_ARCHIVE_H
#define AMBER_ARCHIVE_H

#include "common/archive.h"
#include "common/file.h"
#include "common/hashmap.h"
#include "common/path.h"
#include "common/str.h"

namespace Amber {

class AmberArchive : public Common::Archive {
private:
	struct FileEntry {
		uint16 id;     // The file index (used as the JH decryption key)
		uint32 offset; // position in the physical file
		uint32 size;   // compressed/encrypted size
	};

	Common::File *_file;
	Common::Path _path;
	uint32 _containerType;

	// maps a string filename to its location in the AMNP/AMNC file
	Common::HashMap<Common::String, FileEntry> _entries;

	bool loadStandardTable(uint16 fileCount, int type);
	bool loadSectionTable(uint16 fileCount, int type);

public:
	AmberArchive();
	~AmberArchive() override;

	// reads the AMNC/AMNP header and populates _entries
	bool open(const Common::Path &filename);
	void close();
	bool isOpen() const { return _file->isOpen(); }

	bool hasFile(const Common::Path &path) const override;
	int listMembers(Common::ArchiveMemberList &list) const override;
	const Common::ArchiveMemberPtr getMember(const Common::Path &path) const override;

	Common::SeekableReadStream *createReadStreamForMember(const Common::Path &path) const override;
};
}

#endif // AMBER_ARCHIVE_H
