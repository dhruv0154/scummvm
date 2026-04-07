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

#include "decoders.h"
#include "common/memstream.h"
#include "common/debug.h"
#include "common/textconsole.h"

namespace Amber {

Common::SeekableReadStream *createJHStream(Common::SeekableReadStream *stream, uint16 key, uint32 size) {
	if (!stream || size == 0)
		return nullptr;

	byte *data = (byte *)malloc(size);

	uint32 numWords = size / 2;        // 2 bytes for one word
	bool hasOddByte = (size % 2) != 0; // check if there is 1 leftover byte

	uint16 d0 = key;
	uint16 d1 = 0;
	uint32 offset = 0;

	// read the full 16 bit words one by one
	for (uint32 i = 0; i < numWords; i++) {
		if (stream->eos())
			break;
		uint16 value = stream->readUint16BE();
		value ^= d0;

		data[offset++] = (value >> 8) & 0xFF;
		data[offset++] = value & 0xFF;

		d1 = d0;
		d0 <<= 4;
		d0 = (d0 + d1 + 87) & 0xFFFF;
	}

	// handle the edge case if the file has an odd number of bytes
	if (hasOddByte) {
		uint16 value = stream->readByte() << 8;
		value ^= d0;
		data[offset++] = (value >> 8) & 0xFF;
	}

	return new Common::MemoryReadStream(data, size, DisposeAfterUse::YES);
}


Common::SeekableReadStream *createLOBStream(Common::SeekableReadStream *stream, uint32 decodedSize) {
	if (!stream)
		return nullptr;

	byte *decodedData = (byte *)malloc(decodedSize);
	uint32 decodeIndex = 0;

	while (decodeIndex < decodedSize && !stream->eos()) {
		// the control byte
		// read 1 byte, the 8 bits inside tell us what the next 8 items are [literal or (distance, length)]
		uint8 header = stream->readByte();

		// process all the 8 items for this group
		for (int i = 0; i < 8 && decodeIndex < decodedSize; i++) {

			// we check the msb of the header (if it is 0 the current item is a pointer else it is just a literal)
			if ((header & 0x80) == 0) {
				// the next 2 bytes are containing distance and length
				// in the format: 12 bits for distance 4 bits for length
				// byte1: first 4 bits have distance and next 4 bits have length
				// byte2: contains distance

				// read the first byte
				uint32 matchOffset = stream->readByte();

				// the LOB compression assumes that the minimum length of a compressed string will be 3
				// because it is not worth compressing 1 or 2 characters.
				uint32 matchLength = (matchOffset & 0x0F) + 3;

				// move the top 4 bits of byte1 to the left
				matchOffset <<= 4;

				// mask off the length data in the middle so we are left with distance
				matchOffset &= 0xFF00;

				// read byte2 and merge it in to complete the 12 bit distance number
				matchOffset |= stream->readByte();

				// calculate where in our history to look
				uint32 matchIndex = decodeIndex - matchOffset;

				if (matchOffset > decodeIndex) {
					warning("LOB Decompression error: matchOffset out of bounds!");
					break;
				}

				// copy the old letters forward, one by one, until we hit the length
				while (matchLength-- != 0 && decodeIndex < decodedSize)
					decodedData[decodeIndex++] = decodedData[matchIndex++];
			} else {
				// it's just a regular letter, read 1 byte and save it
				decodedData[decodeIndex++] = stream->readByte();
			}
			// shift the control byte left by 1
			header <<= 1;
		}
	}

	return new Common::MemoryReadStream(decodedData, decodedSize, DisposeAfterUse::YES);
}

}
