#ifndef STORAGE_NODE_H
#define STORAGE_NODE_H

#include <string>

#include "types.h"

struct StorageNodeData
{
	StorageNodeData(): type(0) {}

	StorageNodeData(int type, std::wstring serializedName)
		: type(type), serializedName(std::move(serializedName))
	{
	}

	bool operator<(const StorageNodeData& other) const
	{
		return serializedName < other.serializedName;
	}

	int type;
	std::wstring serializedName;
};

struct StorageNode: public StorageNodeData
{
	StorageNode():  id(0) {}

	StorageNode(Id id, int type, std::wstring serializedName)
		: StorageNodeData(type, std::move(serializedName)), id(id)
	{
	}

	StorageNode(Id id, const StorageNodeData& data): StorageNodeData(data), id(id) {}

	Id id;
};

#endif	  // STORAGE_NODE_H
