#ifndef MURPHY3D_ARCHIVE_H
#define MURPHY3D_ARCHIVE_H

#include "common/scummsys.h"
#include "common/file.h"
#include "common/array.h"
#include "common/stream.h"

namespace Murphy3d {

class Archive {
public:
	Archive();
	~Archive();

	bool open(const Common::Path &filename);
	void close();
	Common::SeekableReadStream *getStream(uint subfile);
	uint getCount() const { return _fileIndex.size(); }

private:
	Common::File _file;
	Common::Array<uint32> _fileIndex;
};

} // End of namespace Murphy3d
#endif
