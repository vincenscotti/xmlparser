#pragma once

#include <string>
#include <map>
#include <vector>

namespace xml
{

class xml_node
{
public:
	std::string name;
	std::map<std::string, std::string> attributes;
	std::vector<xml_node *> children;
	std::string value;
};

class xml_doc
{
public:
	std::string version;
	xml_node *root;

	xml_doc() :
		root{nullptr}
	{

	}
};

}
