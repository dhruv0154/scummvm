#include "common/compression/access_lzw.h"
#include "common/endian.h"
#include "common/file.h"
#include "murphy3d/uakm_dmap.h"

namespace Murphy3d {

bool UAKMDMap::init() {
	Common::File dmapFile;
	if (!dmapFile.open("DMAP.LZ"))
		return false;

	uint32 fileSize = dmapFile.size();
	byte *compressedData = new byte[fileSize];
	dmapFile.read(compressedData, fileSize);
	dmapFile.close();

	byte *decompressedData = nullptr;
	uint32 decompressedSize = Common::decompressAccessDBE(compressedData, &decompressedData);
	delete[] compressedData;

	if (decompressedData == nullptr || decompressedSize == 0)
		return false;

	const byte *data = decompressedData;

	for (int i = 0; i < 64; i++) {
		MapData *pMD = new MapData();
		pMD->scriptFileIndex = 0;
		pMD->scriptFileEntry = 0;
		pMD->locationFileIndex = 0;

		uint32 ptr = READ_LE_UINT32(data + i * 4);
		if (ptr > 0 && ptr < decompressedSize) {
			for (int j = 0; j < 10; j++) {
				if (ptr + 2 > decompressedSize)
					break;
				uint16 w = READ_LE_UINT16(data + ptr);
				ptr += 2;
				if (w == 0xffff)
					break;
			}

			for (int j = 0; j < 99; j++) {
				if (ptr + 2 > decompressedSize)
					break;
				uint16 w = READ_LE_UINT16(data + ptr);
				ptr += 2;
				if (w == 0xffff)
					break;
			}

			ptr += 2;

			if (ptr + 4 <= decompressedSize) {
				uint16 w1 = READ_LE_UINT16(data + ptr);
				ptr += 2;
				uint16 w2 = READ_LE_UINT16(data + ptr);
				(void)w1;
				(void)w2;
				ptr += 2;
				ptr += 4;
			}

			while (ptr < decompressedSize) {
				uint8 a = data[ptr++];
				if (a == 0xff)
					break;

				if (ptr + 4 > decompressedSize)
					break;
				ptr += 4;
			}

			if (ptr + 4 <= decompressedSize) {
				ptr += 4;
			}

			if (ptr + 4 <= decompressedSize) {
				pMD->scriptFileIndex = READ_LE_UINT16(data + ptr);
				ptr += 2;
				pMD->scriptFileEntry = READ_LE_UINT16(data + ptr);
				ptr += 2;
			}

			while (ptr + 2 <= decompressedSize) {
				uint16 fi1 = READ_LE_UINT16(data + ptr);
				ptr += 2;
				if (fi1 == 0xffff)
					break;

				if (ptr + 2 > decompressedSize)
					break;
				uint16 fe1 = READ_LE_UINT16(data + ptr);
				ptr += 2;

				FileMap fm;
				fm.file = fi1;
				fm.entry = fe1;
				pMD->videoMap.push_back(fm);

				if (ptr + 4 > decompressedSize)
					break;
				uint16 fi2 = READ_LE_UINT16(data + ptr);
				ptr += 2;
				uint16 fe2 = READ_LE_UINT16(data + ptr);
				ptr += 2;

				FileMap afm;
				afm.file = fi2;
				afm.entry = fe2;
				pMD->audioMap.push_back(afm);
			}

			while (ptr + 2 <= decompressedSize) {
				FileMap fm;
				fm.file = READ_LE_UINT16(data + ptr);
				ptr += 2;
				if (fm.file == 0xffff)
					break;

				if (ptr + 2 > decompressedSize)
					break;
				fm.entry = READ_LE_UINT16(data + ptr);
				ptr += 2;
				pMD->imageMap.push_back(fm);
			}
		}

		_entries.push_back(pMD);
	}

	delete[] decompressedData;
	return true;
}

} // End of namespace Murphy3d
