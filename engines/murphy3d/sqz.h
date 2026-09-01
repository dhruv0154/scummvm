#ifndef MURPHY3D_SQZ_H
#define MURPHY3D_SQZ_H

#include "common/scummsys.h"
#include "murphy3d/location.h"

namespace Murphy3d {

struct BinaryData {
	uint8 *data;
	int length;
};

class SQZ {
public:
	SQZ();
	~SQZ();

	static BinaryData decompress(uint8 *input, int length);
};

} // End of namespace Murphy3d
#endif
