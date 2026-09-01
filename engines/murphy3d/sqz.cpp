#include "murphy3d/sqz.h"
#include "murphy3d/location.h"

namespace Murphy3d {

SQZ::SQZ() {
}

SQZ::~SQZ() {
}

BinaryData SQZ::decompress(uint8 *input, int length) {
	// Must start with '.SQZ'
	// Looks like it is always followed by '.bmp'
	// Compressed size
	// Decompressed size

	BinaryData bd;
	bd.data = nullptr;
	bd.length = 0;

	if (length >= 16 &&
		input[0] == '.' && input[1] == 'S' && input[2] == 'Q' && input[3] == 'Z' &&
		input[4] == '.' && input[5] == 'b' && input[6] == 'm' && input[7] == 'p') {
		uint32 compressedSize = READ_LE_UINT32(input + 8);
		uint32 decompressedSize = READ_LE_UINT32(input + 12);

		uint8_t *output = new uint8_t[decompressedSize];
		if (output != NULL) {
			memset(output, 0, decompressedSize);

			bd.data = output;
			bd.length = decompressedSize;

			int src = 16;
			int dst = 0;

			while (src < length) {
				uint16 chunkSize = READ_LE_UINT16(input + src);
				src += 2;
				if ((chunkSize & 0x8000) != 0) {
					// Direct copy
					chunkSize = (-chunkSize) & 0xffff;

					for (int cp = 0; cp < chunkSize; cp++) {
						output[dst++] = input[src++];
					}
				} else {
					// chunkSize bytes are compressed, decompress them...
					int chunkEnd = src + chunkSize;
					while (src < chunkEnd) {
						int dataCount = input[src++];
						if ((dataCount & 0x80) != 0) {
							// Signed
							int count = 0, offset = 0;
							int lowCount = dataCount & 0x7f;
							int additional = input[src++];
							if ((additional & 0x80) != 0) {
								// Signed
								count = (lowCount >> 4) + 3;
								offset = ((additional & 0x7f) << 4) | (lowCount & 0xf);
							} else {
								// Unsigned
								count = lowCount + 3;
								offset = (additional << 8) | input[src++];
							}

							for (int cp = 0; cp < count; cp++) {
								output[dst] = output[dst - offset];
								dst++;
							}
						} else {
							// Unsigned, copy uncompressed chunk data
							dataCount++;
							for (int cp = 0; cp < dataCount; cp++) {
								output[dst++] = input[src++];
							}
						}
					}
				}
			}
		}
	}

	return bd;
}

} // End of namespace Murphy3d
