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

#include "common/stream.h"
#include "graphics/surface.h"

#include "mutationofjb/encryptedfile.h"
#include "mutationofjb/video/mojb_decoder.h"

namespace MutationOfJB {

#define FLI_FILE_HEADER       0xAF11
#define FLI_FILE_HEADER_SIZE  0x80
#define FLI_IMAGE_WIDTH       320
#define FLI_IMAGE_HEIGHT      200

bool MojbDecoder::loadFile(const Common::Path &filename) {
	EncryptedFile *file = new EncryptedFile();

	if (!file->open(filename)) {
		delete file;
		return false;
	}

	bool result = loadStream(file);
	if (!result)
		delete file;
	return result;
}

bool MojbDecoder::loadStream(Common::SeekableReadStream *stream) {
	close();

	/* uint32 frameSize = */ stream->readUint32LE();
	uint16 frameType = stream->readUint16LE();

	// Check FLC magic number
	if (frameType != FLI_FILE_HEADER) {
		warning("MojbDecoder::loadStream(): attempted to load non-FLI data (type = 0x%04X)", frameType);
		return false;
	}

	uint16 frameCount = stream->readUint16LE();
	uint16 width = stream->readUint16LE();
	if (width != FLI_IMAGE_WIDTH) {
		warning("MojbDecoder::loadStream(): attempted to load an FLI with non-standard width (width = %d)", width);
		return false;
	}

	uint16 height = stream->readUint16LE();
	if (height != FLI_IMAGE_HEIGHT) {
		warning("MojbDecoder::loadStream(): attempted to load an FLI with non-standard height (height = %d)", height);
		return false;
	}

	uint16 colorDepth = stream->readUint16LE();
	if (colorDepth != 8) {
		warning("MojbDecoder::loadStream(): attempted to load an FLI with a palette of color depth %d. Only 8-bit color palettes are supported", colorDepth);
		return false;
	}

	addTrack(new MojbVideoTrack(stream, frameCount, width, height));

	return true;
}

MojbDecoder::MojbVideoTrack::MojbVideoTrack(Common::SeekableReadStream *stream, uint16 frameCount, uint16 width, uint16 height) :
	Video::FlicDecoder::FlicVideoTrack(stream, frameCount, width, height, true) {
	readHeader();
}

void MojbDecoder::MojbVideoTrack::readHeader() {
	_fileStream->readUint16LE();	// flags
	// Note: The normal delay is a 32-bit integer (dword), whereas the overridden delay is a 16-bit integer (word)
	// the frame delay is the FLIC "speed", in increments of 1/70 second.
	_frameDelay = _startFrameDelay = _fileStream->readUint32LE() * 1000 / 70;

	_fileStream->seek(80);
	_offsetFrame1 = FLI_FILE_HEADER_SIZE; // In FLI files the first frame is assumed to start directly behind the file header
	_offsetFrame2 = 0; // FLI files don't have a ring frame and so this shouldn't be needed at all, rewind will always seek to _offsetFrame1

	// Seek to the first frame
	_fileStream->seek(_offsetFrame1);
}

bool MojbDecoder::MojbVideoTrack::rewind() {
	_atRingFrame = false; // Original FLI file doesn't have a ring frame
	_fileStream->seek(_offsetFrame1);
	_curFrame = -1;
	_nextFrameStartTime = 0;
	_frameDelay = _startFrameDelay;
	return true;
}

#define FLI_COLOR             11
#define FLI_LC                12
#define FLI_BLACK             13
#define FLI_BRUN              15
#define FLI_COPY              16

void MojbDecoder::MojbVideoTrack::handleFrame() {
	uint16 chunkCount = _fileStream->readUint16LE();
	_fileStream->readUint64LE(); // Next 8 Reserved in FLI file files

	// Read subchunks
	for (uint32 i = 0; i < chunkCount; ++i) {
		uint32 frameSize = _fileStream->readUint32LE();
		uint16 frameType = _fileStream->readUint16LE();
		uint8 *data = new uint8[frameSize - 6];
		_fileStream->read(data, frameSize - 6);

		switch (frameType) {
		case FLI_COLOR:
			decodeColorPalette(data);
			_dirtyPalette = true;
			break;
		case FLI_LC:
			decodeDeltaFLI(data);
			break;
		case FLI_BLACK:
			_surface->fillRect(Common::Rect(0, 0, getWidth(), getHeight()), 0);
			_dirtyRects.clear();
			_dirtyRects.push_back(Common::Rect(0, 0, getWidth(), getHeight()));
			break;
		case FLI_BRUN:
			decodeByteRun(data);
			break;
		case FLI_COPY:
			copyFrame(data);
			break;
		default:
			error("MojbDecoder::handleFrame(): unknown subchunk type (type = 0x%02X)", frameType);
			break;
		}

		delete[] data;
	}
}

void MojbDecoder::MojbVideoTrack::decodeDeltaFLI(uint8 *data) {
	uint16 currentLine = READ_LE_UINT16(data); data += 2;
	uint16 linesInChunk = READ_LE_UINT16(data); data += 2;
	
	while (linesInChunk--) {
		uint8 packetCount = *data++;
		uint16 column = 0;

		// Now interpret the RLE data
		while (packetCount--) {
			column += *data++;
			int8 rleCount = (int8)*data++;
			
			if (rleCount > 0) {
				memcpy((byte *)_surface->getBasePtr(column, currentLine), data, rleCount);
				data += rleCount;
				_dirtyRects.push_back(Common::Rect(column, currentLine, column + rleCount, currentLine + 1));
			} else if (rleCount < 0) {
				rleCount = -rleCount;
				uint8 dataByte = *data++;
				for (uint8 i = 0; i < rleCount; ++i) {
					*((byte *)_surface->getBasePtr(column + i, currentLine)) = dataByte;
				}
				_dirtyRects.push_back(Common::Rect(column, currentLine, column + rleCount, currentLine + 1));
			}
			column += rleCount;
		}

		currentLine++;
	}
}

void MojbDecoder::MojbVideoTrack::decodeColorPalette(uint8 *data) {
	uint16 numPackets = READ_LE_UINT16(data); data += 2;
	
	// FLI files use a 6-bit palette and not a 8-bit one, we change it to 8-bit
	if (0 == READ_LE_UINT16(data)) { //special case
		data += 2;
		for (int i = 0; i < 256; ++i) {
			byte r = data[i * 3] & 0x3F;
			byte g = data[i * 3 + 1] & 0x3F;
			byte b = data[i * 3 + 2] & 0x3F;
			_palette.set(i, (r << 2) | (r >> 4), (g << 2) | (g >> 4), (b << 2) | (b >> 4));
		}
	} else {
		uint8 palPos = 0;

		while (numPackets--) {
			palPos += *data++;
			uint8 change = *data++;

			for (int i = 0; i < change; ++i) {
				byte r = data[i * 3] & 0x3F;
				byte g = data[i * 3 + 1] & 0x3F;
				byte b = data[i * 3 + 2] & 0x3F;
				_palette.set(palPos + i, (r << 2) | (r >> 4), (g << 2) | (g >> 4), (b << 2) | (b >> 4));
			}

			palPos += change;
			data += (change * 3);
		}
	}
}

} // End of namepsace MutationOfJB
