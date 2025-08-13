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

#include "mutationofjb/inventoryitemdefinitionlist.h"

#include "mutationofjb/game.h"
#include "mutationofjb/encryptedfile.h"
#include "mutationofjb/util.h"

namespace MutationOfJB {

InventoryItemDefinitionList::InventoryItemDefinitionList(Game &game) {
	parseFile(game.getLanguage());
}

int InventoryItemDefinitionList::findItemIndex(const Common::String &itemName) const {
	const InventoryItemMap::const_iterator it = _inventoryItemMap.find(itemName);
	if (it == _inventoryItemMap.end())
		return -1;
	return it->_value;
}

const Common::String &InventoryItemDefinitionList::getItemName(const Common::String &itemName) const {
	const int itemIndex = findItemIndex(itemName);
	const InventoryItemNameMap::const_iterator it = _inventoryItemNamesMap.find(itemIndex);

	return it->_value;
}

bool InventoryItemDefinitionList::parseFile(Common::Language lang) {
	EncryptedFile file;
	const char *fileName = "fixitems.dat";
	file.open(fileName);
	if (!file.isOpen()) {
		reportFileMissingError(fileName);
		return false;
	}

	int itemIndex = 0;
	while (!file.eos()) {
		Common::String line = file.readLine();
		if (line.empty() || line.hasPrefix("#")) {
			continue;
		}

		Common::String::iterator firstSpace = Common::find(line.begin(), line.end(), ' ');
		if (firstSpace == line.end()) {
			continue;
		}
		const int len = firstSpace - line.begin();
		if (!len) {
			continue;
		}
		Common::String item(line.c_str(), len);
		_inventoryItemMap[item] = itemIndex;

		// Get the translated item name..
		Common::String translatedItemName = parseTranslatedItemName(line, lang);
		if (translatedItemName.empty())
			translatedItemName = Common::String(line.c_str());

		_inventoryItemNamesMap[itemIndex] = translatedItemName;
		itemIndex++;
	}

	return true;
}

Common::String InventoryItemDefinitionList::parseTranslatedItemName(const Common::String &itemLine, Common::Language lang) {
	if (itemLine.empty() || itemLine.hasPrefix("#")) {
		return Common::String();
	}

	uint spaceCounter = 0;
	uint previousSpacePos = 0;
	int nextSpacePos = 0;

	do {
		nextSpacePos = itemLine.find(' ', previousSpacePos + 1);
		if (nextSpacePos > -1) {
			if (previousSpacePos == 0) { // first part is the general item name which we'll skip
				previousSpacePos = nextSpacePos;
				spaceCounter++;
				continue;
			}

			int translatedItemNameLength = nextSpacePos - previousSpacePos - 1;

			if (lang == Common::SK_SVK && spaceCounter == 1) // string after first space is SK item name
				return itemLine.substr(previousSpacePos + 1, translatedItemNameLength);
			else if (lang == Common::DE_DEU && spaceCounter == 3) // string after third space is DE item name
				return itemLine.substr(previousSpacePos + 1, translatedItemNameLength);

			spaceCounter++;
			previousSpacePos = nextSpacePos;
		}
	} while (nextSpacePos > -1 && previousSpacePos + 1 < itemLine.size());

	return Common::String();
}

}
