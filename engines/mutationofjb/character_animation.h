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

#ifndef MUTATIONOFJB_CHARACTER_ANIMATION_H
#define MUTATIONOFJB_CHARACTER_ANIMATION_H

#include "graphics/palette.h"

#include "mutationofjb/encryptedfile.h"

namespace MutationOfJB {

struct AnimationFrame {
	uint8 *_data;
	uint16 _dataSize;
	uint8 *_transformedData;
	uint16 _transformedDataSize;
	uint8 height;
	uint8 width;
};

class Animation {
public:
	Animation();
	~Animation();
	
	bool loadAnimation(const Common::Path &fileName);
	AnimationFrame *_frames;
	uint16 _numFrames;
	Graphics::Palette _palette;
	
private:
	void decodeColorPalette(uint8 *data);
	void decodeRLEData(const uint8 *inputData, uint16 inputLength, AnimationFrame *frame);
	void transformData(const uint8 *inputData, uint16 inputLength, AnimationFrame *frame);
};

}

#endif
