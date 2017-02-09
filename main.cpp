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
	for (char c : str) {
		if (s == e || *s != c) {
			throw no_match{};
		}

		s++;
	}
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
	try {
		whitespaces(s, e);
	} catch (const no_match &) {

	}

	match_char(s, e, '=');

	try {
		whitespaces(s, e);
	} catch (const no_match &) {

	}
}

template <typename It>
std::string version_info(It &s, const It &e)
{
	whitespaces(s, e);
	match_string(s, e, "version");
	equals(s, e);

	auto curr = s;

	try {
		match_string(curr, e, "'1.0'");
	} catch (const no_match &) {
		curr = s;
		match_string(curr, e, "\"1.0\"");
	}

	s = curr;

	return "1.0";
}

template <typename It>
std::string xml_decl(It &s, const It &e)
{
	std::string version;
	auto first_match = "<?xml";
	auto last_match = "?>";

	match_string(s, e, first_match);
	version = version_info(s, e);

	try {
		whitespaces(s, e);
	} catch (const no_match &) {

	}

	match_string(s, e, last_match);

	return version;
}

template <typename It>
xml_doc prologue(It &s, const It &e)
{
	xml_doc doc;

	doc.version = xml_decl(s, e);

	try {
		whitespaces(s, e);
	} catch (const no_match &) {

	}

	return doc;
}

template <typename It>
std::string name(It &s, const It &e)
{
	std::string ret;
	char c;

	if (s == e) {
		throw no_match{};
	}

	c = *s;
	if (!std::isalpha(c) && c != '_' && c != ':') {
		throw no_match{};
	}

	ret += c;
	s++;

	while (s != e) {
		c = *s;

		if (!std::isalnum(c) && c != '.' && c != ':' && c != '-' && c != '_') {
			break;
		}

		ret += c;
		s++;
	}

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
	std::string ret;
	char quote_char = '"';
	char c;

	try {
		match_char(s, e, quote_char);
	} catch (const no_match &) {
		quote_char = '\'';
		match_char(s, e, quote_char);
	}

	while (s != e) {
		c = *s;

		if (c == quote_char) {
			s++;
			break;
		}

		ret += c;
		s++;
	}

	return ret;
}

template <typename It>
std::pair<std::string, std::string> attribute(It &s, const It &e)
{
	std::pair<std::string, std::string> ret;

	ret.first = name(s, e);
	equals(s, e);
	ret.second = attribute_value(s, e);

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
	xml_node *ret = new xml_node;
	bool empty_node;

	try {
		match_char(s, e, '<');
		ret->name = name(s, e);

		while (true) {
			try {
				whitespaces(s, e);
//				ret->attributes.insert(attribute(curr, e));
				ret->attributes.insert(attribute(s, e));
			} catch (const no_match &) {
				break;
			}
		}

		try {
			auto curr = s;

			match_char(curr, e, '/');
			empty_node = true;

			s = curr;
		} catch (const no_match &) {
			empty_node = false;
		}

		match_char(s, e, '>');

		if (empty_node) {
			return ret;
		}

		while (true) {
			try {
				whitespaces(s, e);
			} catch (const no_match &) {

			}

			try {
				auto curr = s;

				ret->children.push_back(element(curr, e));

				s = curr;
			} catch (const no_match &) {
				break;
			}
		}

		if (ret->children.empty()) {
			try {
				ret->value = chardata(s, e);
				rtrim(ret->value);
			} catch (const no_match &) {

			}
		}

		match_char(s, e, '<');
		match_char(s, e, '/');
		match_string(s, e, ret->name);

		try {
			whitespaces(s, e);
		} catch (const no_match &) {

		}

		match_char(s, e, '>');
	} catch (const no_match &) {
		delete ret;
		throw;
	}

	return ret;
}

template <typename It>
xml_doc document(It &s, const It &e)
{
	xml_doc ret;

	try {
		try {
			auto curr = s;

			ret = prologue(curr, e);

			s = curr;
		} catch (const no_match &ex) {

		}

		ret.root = element(s, e);
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
