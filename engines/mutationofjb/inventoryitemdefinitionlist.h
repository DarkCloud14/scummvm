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

#ifndef MUTATIONOFJB_INVENTORYITEMDEFINITIONLIST_H
#define MUTATIONOFJB_INVENTORYITEMDEFINITIONLIST_H

#include "common/hash-str.h"
#include "common/hashmap.h"
#include "common/language.h"

namespace MutationOfJB {

class Game;

typedef Common::HashMap<Common::String, int> InventoryMap;
typedef Common::HashMap<int, Common::String> InventoryItemNameMap;

class InventoryItemDefinitionList {
public:
	InventoryItemDefinitionList(Game &game);
	const InventoryMap &getInventoryMap() const;

	int findItemIndex(const Common::String &itemName) const;
	const Common::String &getItemName(const Common::String &itemName) const;

private:
	bool parseFile(Common::Language lang);
	Common::String parseTranslatedItemName(const Common::String &itemLine, Common::Language lang);

	typedef Common::HashMap<Common::String, int> InventoryItemMap;
	InventoryItemMap _inventoryItemMap;

	typedef Common::HashMap<int, Common::String> InventoryItemNameMap;
	InventoryItemNameMap _inventoryItemNamesMap;
};

}

#endif
