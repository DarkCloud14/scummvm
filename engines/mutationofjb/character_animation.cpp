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

#include "mutationofjb/character_animation.h"
#include "mutationofjb/encryptedfile.h"
#include "mutationofjb/util.h"

namespace MutationOfJB {

#define COLOR_PALETTE_POSITION 0x06
#define COLOR_PALETTE_SIZE     16
#define FRAME_COUNT_POSITION   0x02D6
#define FRAME_HEIGHT           59
#define FRAME_WIDTH            60

bool Animation::loadAnimation(const Common::Path &fileName) {
	EncryptedFile file;
	file.open(fileName);

	if (!file.isOpen()) {
		reportFileMissingError(fileName.toString(Common::Path::kNativeSeparator).c_str());
		return false;
	}
	
	file.readByte(); // Skip first byte
	Common::String fileIdentifier = file.readString('\0', 5);
	if (!fileIdentifier.equals("ASANP"))
		return false;

	file.seek(COLOR_PALETTE_POSITION, SEEK_SET);
	uint8 colorPaletteDataSize = COLOR_PALETTE_SIZE * 3;
	uint8 *data = new uint8[colorPaletteDataSize];
	file.read(data, colorPaletteDataSize);
	decodeColorPalette(data);
	delete[] data;

	file.seek(FRAME_COUNT_POSITION, SEEK_SET);
	_numFrames = file.readUint16LE();

	delete[] _frames;
	_frames = new AnimationFrame[_numFrames];
	uint16 *packedFrameSizes = new uint16[_numFrames];
	for (uint16 i = 0; i < _numFrames; i++) {
		packedFrameSizes[i] = file.readUint16LE();
	}
	
	// Now that we know the packed size of each animation frame we can start
	// reading and decoding the particular animation data
	for (uint16 i = 0; i < _numFrames; i++) {
		_frames[i].height = FRAME_HEIGHT;
		_frames[i].width = FRAME_WIDTH;

		uint16 packedFrameSize = packedFrameSizes[i];
		uint8 *data = new uint8[packedFrameSize];
		file.read(data, packedFrameSize);
		decodeRLEData(data, packedFrameSize, &_frames[i]);
		transformData(_frames[i]._data, _frames[i]._dataSize, &_frames[i]);
	}

	return true;
}

Animation::Animation() : _palette(COLOR_PALETTE_SIZE) {
	_numFrames = 0;
	_frames = nullptr;
}

Animation::~Animation() {
	for (uint16 i = 0; i < _numFrames; i++) {
		delete[] _frames[i]._data;
	}
	delete[] _frames;
}

void Animation::decodeRLEData(const uint8 *inputData, uint16 inputLength, AnimationFrame *frame) {
	frame->_dataSize = frame->width * frame->height;
	frame->_data = new uint8[frame->_dataSize];
	uint8 *dataPointer = frame->_data;
	memset((uint8 *)dataPointer, 0xAF, frame->_dataSize);

	for (uint16 i = 0; i < inputLength; i += 2) {
		uint8 runCount = (uint8)*inputData++;
		uint8 colorPaletteIndex = (uint8)(*inputData++ & (COLOR_PALETTE_SIZE - 1));
		memset((uint8 *)dataPointer, colorPaletteIndex, runCount);
		dataPointer += runCount;
	}
}

void Animation::decodeColorPalette(uint8 *data) {
	for (uint8 i = 0; i < COLOR_PALETTE_SIZE; ++i) {
		byte r = data[i * 3] & 0x3F;
		byte g = data[i * 3 + 1] & 0x3F;
		byte b = data[i * 3 + 2] & 0x3F;
		_palette.set(i, (r << 2) | (r >> 4), (g << 2) | (g >> 4), (b << 2) | (b >> 4));
	}
}

void Animation::transformData(const uint8 *inputData, uint16 inputLength, AnimationFrame *frame) {
	frame->_transformedDataSize = inputLength / 2;
	frame->_transformedData = new uint8[frame->_transformedDataSize];
	uint8 *dataPointer = frame->_transformedData;
	memset((uint8 *)dataPointer, 0xAF, frame->_transformedDataSize);

	for (uint16 i = 0; i < inputLength; i += 2) {
		uint8 firstByteToTransform = (uint8)*inputData++;
		uint8 secondByteToTransform = (uint8)*inputData++;

		if (firstByteToTransform != 0) {
			firstByteToTransform -= 0x0F;
		}

		if (secondByteToTransform != 0) {
			secondByteToTransform -= 0x0F;
			secondByteToTransform <<= 4;
		}
		
		uint8 transformedByte = firstByteToTransform + secondByteToTransform;
		memset((uint8 *)dataPointer, transformedByte, 1);
		dataPointer++;
	}
}

}
