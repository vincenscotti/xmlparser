#include <iostream>
#include <fstream>
#include <iterator>
#include <exception>
#include <algorithm>
#include <string>
#include <map>

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
		root(nullptr)
	{

	}
};

namespace parsers
{

class no_match : public std::exception
{
public:
	virtual const char *what() const noexcept override
	{
		return "no match!";
	}
};

template <typename It>
void match_char(It &s, const It &e, char c)
{
	if (s == e || *s != c) {
		throw no_match{};
	}

	s++;
}

template <typename It>
void match_string(It &s, const It &e, const std::string &str)
{
	auto curr = s;

	for (char c : str) {
		if (curr == e || *curr != c) {
			throw no_match{};
		}

		curr++;
	}

	s = curr;
}

template <typename It>
void whitespaces(It &s, const It &e)
{
	if (s == e || !std::isspace(*s)) {
		throw no_match{};
	}

	do {
		s++;
	} while (s != e && std::isspace(*s));
}

template <typename It>
void equals(It &s, const It &e)
{
	auto curr = s;

	try {
		whitespaces(curr, e);
	} catch (const no_match &) {

	}

	match_char(curr, e, '=');

	try {
		whitespaces(curr, e);
	} catch (const no_match &) {

	}

	s = curr;
}

template <typename It>
std::string version_info(It &s, const It &e)
{
	auto curr = s;

	whitespaces(curr, e);
	match_string(curr, e, "version");
	equals(curr, e);
	try {
		match_string(curr, e, "'1.0'");
	} catch (const no_match &) {
		match_string(curr, e, "\"1.0\"");
	}

	s = curr;

	return "1.0";
}

template <typename It>
std::string xml_decl(It &s, const It &e)
{
	std::string version;
	auto curr = s;
	auto first_match = "<?xml";
	auto last_match = "?>";

	match_string(curr, e, first_match);
	version = version_info(curr, e);

	try {
		whitespaces(curr, e);
	} catch (const no_match &) {

	}

	match_string(curr, e, last_match);

	s = curr;

	return version;
}

template <typename It>
xml_doc prologue(It &s, const It &e)
{
	xml_doc doc;

	auto curr = s;

	try {
		doc.version = xml_decl(curr, e);
	} catch (const no_match &) {
		doc.version = "1.0";
	}

	try {
		whitespaces(curr, e);
	} catch (const no_match &) {

	}

	s = curr;

	return doc;
}

template <typename It>
std::string name(It &s, const It &e)
{
	auto curr = s;
	std::string ret;
	char c;

	if (s == e) {
		throw no_match{};
	}

	c = *curr;
	if (!std::isalpha(c) && c != '_' && c != ':') {
		throw no_match{};
	}

	ret += c;
	curr++;

	while (curr != e) {
		c = *curr;

		if (!std::isalnum(c) && c != '.' && c != ':' && c != '-' && c != '_') {
			break;
		}

		ret += c;
		curr++;
	}

	s = curr;

	return ret;
}

template <typename It>
std::string chardata(It &s, const It &e)
{
	std::string ret;
	char c;

	if (s == e || *s == '<') {
		throw no_match{};
	}

	while (s != e) {
		c = *s;

		if (c == '<') {
			break;
		}

		ret += c;
		s++;
	}

	return ret;
}

template <typename It>
std::string attribute_value(It &s, const It &e)
{
	auto curr = s;

	std::string ret;
	char quote_char = '"';
	char c;

	try {
		match_char(curr, e, quote_char);
	} catch (const no_match &) {
		quote_char = '\'';
		match_char(curr, e, quote_char);
	}

	while (curr != e) {
		c = *curr;

		if (c == quote_char) {
			curr++;
			break;
		}

		ret += c;
		curr++;
	}

	s = curr;

	return ret;
}

template <typename It>
std::pair<std::string, std::string> attribute(It &s, const It &e)
{
	auto curr = s;

	std::pair<std::string, std::string> ret;

	ret.first = name(curr, e);
	equals(curr, e);
	ret.second = attribute_value(curr, e);

	s = curr;

	return ret;
}

void rtrim(std::string &s)
{
	auto first_non_space = std::find_if(s.rbegin(), s.rend(), [] (char c) {
		return !std::isspace(c);
	});

	s.resize(s.size() - std::distance(s.rbegin(), first_non_space));
}

template <typename It>
xml_node *element(It &s, const It &e)
{
	auto curr = s;

	xml_node *ret = new xml_node;
	bool empty_node;

	try {
		match_char(curr, e, '<');
		ret->name = name(curr, e);

		while (true) {
			try {
				whitespaces(curr, e);
				ret->attributes.insert(attribute(curr, e));
			} catch (const no_match &) {
				break;
			}
		}

		try {
			match_char(curr, e, '/');
			empty_node = true;
		} catch (const no_match &) {
			empty_node = false;
		}

		match_char(curr, e, '>');

		if (empty_node) {
			s = curr;
			return ret;
		}

		while (true) {
			try {
				whitespaces(curr, e);
			} catch (const no_match &) {

			}

			try {
				ret->children.push_back(element(curr, e));
			} catch (const no_match &) {
				break;
			}
		}

		if (ret->children.empty()) {
			try {
				ret->value = chardata(curr, e);
				rtrim(ret->value);
			} catch (const no_match &) {

			}
		}

		match_char(curr, e, '<');
		match_char(curr, e, '/');
		match_string(curr, e, ret->name);

		try {
			whitespaces(curr, e);
		} catch (const no_match &) {

		}

		match_char(curr, e, '>');
	} catch (const no_match &) {
		delete ret;
		throw;
	}

	s = curr;

	return ret;
}

template <typename It>
xml_doc document(It &s, const It &e)
{
	auto curr = s;

	xml_doc ret;

	try {
		ret = prologue(curr, e);
		ret.root = element(curr, e);

		s = curr;
	} catch (const no_match &ex) {
		std::cerr << ex.what() << std::endl;
	}

	return ret;
}

}

void dump(const xml_node *node, int depth = 0)
{
	if (node == nullptr)
		return;

	for (int i = 0; i < depth; i++) {
		std::cout << " ";
	}

	std::cout << node->name << " [";

	for (auto &p : node->attributes) {
		std::cout << p.first << "=" << p.second << ", ";
	}

	std::cout << "]";

	if (!node->value.empty()) {
		std::cout << " = " << node->value;
	}

	std::cout << std::endl;

	for (auto &c : node->children) {
		dump(c, depth + 1);
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

	xml_doc doc;

	doc = parsers::document(start, end);

	std::cout << "xml version: " << doc.version << std::endl;
	dump(doc.root);

	std::cout << "chars left: " << std::distance(start, end) << std::endl;
	std::for_each(start, end, [] (char c) {
		std::cout << c;
	});

	std::cout << std::endl;

	return 0;
}
