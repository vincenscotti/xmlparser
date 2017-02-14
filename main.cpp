#include <iostream>
#include <fstream>
#include <iterator>
#include <algorithm>

#include "xml.h"
#include "xml_parser.h"
#include "xml_parser_error.h"

void print_node(std::ostream &os, const xml::xml_node *node, int depth = 0)
{
	if (node == nullptr)
		return;

	for (int i = 0; i < depth; i++) {
		os << " ";
	}

	os << node->name << " [";

	for (auto &p : node->attributes) {
		os << p.first << "=" << p.second << ", ";
	}

	os << "]";

	if (!node->value.empty()) {
		os << " = " << node->value;
	}

	os << std::endl;

	for (auto &c : node->children) {
		print_node(os, c, depth + 1);
	}
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		return 1;
	}

	std::ifstream infile(argv[1]);
	infile >> std::noskipws;

	std::istream_iterator<char> file_it(infile), eof;

	std::string buffer(file_it, eof);
	auto start = buffer.begin();
	const auto end = buffer.end();

	xml::xml_doc doc;

	try {
		doc = xml::parse(start, end);
	} catch (const xml::parser_error &ex) {
		std::cerr << ex.what() << std::endl << std::endl;

		std::cerr << "Chars left: " << std::distance(start, end) << std::endl << std::endl;
		std::for_each(start, end, [] (char c) {
			std::cerr << c;
		});
		std::cerr << std::endl;

		return 1;
	}

	std::cout << "XML version: " << doc.version << std::endl << std::endl;
	print_node(std::cout, doc.root);
	std::cout << std::endl;

	return 0;
}
