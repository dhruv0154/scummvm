#include "murphy3d/uakm_map.h"
#include "common/compression/access_lzw.h"
#include "common/endian.h"
#include "common/file.h"

namespace Murphy3d {

bool UAKMMap::init() {
	Common::File mapFile;
	if (!mapFile.open("MAP.LZ"))
		return false;

	uint32 fileSize = mapFile.size();
	byte *compressedData = new byte[fileSize];
	mapFile.read(compressedData, fileSize);
	mapFile.close();

	byte *decompressedData = nullptr;
	// decompress the MAP.LZ file following the DBE format
	uint32 decompressedSize = Common::decompressAccessDBE(compressedData, &decompressedData);
	delete[] compressedData;

	if (decompressedData == nullptr || decompressedSize == 0)
		return false;

	const byte *data = decompressedData;

	for (int i = 0; i < 64; i++) {
		MapData *pMD = new MapData();

		// 32-bit value * 64: offsets to location media map entries (value of 0 means entry not in use)
		uint32 ptr = READ_LE_UINT32(data + i * 4);
		if (ptr > 0 && ptr < decompressedSize) {

			// Up to 10 values or -1 to indicate end of list
			for (int j = 0; j < 10; j++) {
				uint16 w = READ_LE_UINT16(data + ptr);
				ptr += 2;
				if (w == 0xffff)
					break;
			}

			// Up to 99 values or -1 to indicate end of list
			for (int j = 0; j < 99; j++) {
				uint16 w = READ_LE_UINT16(data + ptr);
				ptr += 2;
				if (w == 0xffff)
					break;
			}
			ptr += 6;

			// script file index
			pMD->scriptFileIndex = READ_LE_UINT16(data + ptr);
			ptr += 2;
			// script file entry
			pMD->scriptFileEntry = READ_LE_UINT16(data + ptr);
			ptr += 2;

			// audio map 1
			while (ptr < decompressedSize) {
				FileMap s;
				s.file = READ_LE_UINT16(data + ptr);
				ptr += 2;
				if (s.file == 0xffff)
					break;
				s.entry = READ_LE_UINT16(data + ptr);
				ptr += 2;
				pMD->environmentAudioMap.push_back(s);
			}

			// audio map 2
			while (ptr < decompressedSize) {
				FileMap fm;
				fm.file = READ_LE_UINT16(data + ptr);
				ptr += 2;
				if (fm.file == 0xffff)
					break;
				fm.entry = READ_LE_UINT16(data + ptr);
				ptr += 2;
				pMD->audioMap.push_back(fm);
			}

			// video map
			while (ptr < decompressedSize) {
				FileMap fm;
				fm.file = READ_LE_UINT16(data + ptr);
				ptr += 2;
				if (fm.file == 0xffff)
					break;
				fm.entry = READ_LE_UINT16(data + ptr);
				ptr += 2;
				pMD->videoMap.push_back(fm);
			}

			// image map
			while (ptr < decompressedSize) {
				FileMap fm;
				fm.file = READ_LE_UINT16(data + ptr);
				ptr += 2;
				if (fm.file == 0xffff)
					break;
				fm.entry = READ_LE_UINT16(data + ptr);
				ptr += 2;
				pMD->imageMap.push_back(fm);
			}

			// 16-bit value, location file index
			pMD->locationFileIndex = READ_LE_UINT16(data + ptr);
			ptr += 4;

			// animation map
			while (ptr < decompressedSize) {
				uint8 u = data[ptr++];
				if (u == 0xff)
					break;
				pMD->animationMap.push_back(u);
			}

			// interactive object map
			while (ptr < decompressedSize) {
				uint32 u = READ_LE_UINT32(data + ptr);
				ptr += 4;
				if (u == 0xffffffff)
					break;
				pMD->objectMap.push_back(u);
			}

			if (i < 43) {
				uint32 startPosPtr = READ_LE_UINT32(data + 0x100 + i * 4);
				if (startPosPtr > 0 && startPosPtr < decompressedSize) {
					uint32 nextPtr = READ_LE_UINT32(data + 0x104 + i * 4);
					if (nextPtr != 0 && nextPtr < decompressedSize) {
						int entries = (nextPtr - startPosPtr) / 10;
						readStartupPositions(pMD, data, startPosPtr, entries, 10);
					}
				}
			}
		}
		_entries.push_back(pMD);
	}

	delete[] decompressedData;
	return true;
}

} // End of namespace Murphy3d
