#pragma once
#include "../Attributes.h"

class BaseAttributeMap {
public:
	std::unordered_map<int, AttributeInstance> attributes;
	std::vector<uint64_t> badAttributes;
};
struct AttributesComponent {
	BaseAttributeMap baseMap;
};