#pragma once

#include "xml.h"
#include "xml_parser_error.h"

namespace xml
{
namespace parser
{

template <typename It>
void match_char(It &s, const It &e, char c)
{
	if (s == e || *s != c) {
		throw parser_error{std::string{c}};
	}

	s++;
}

template <typename It>
void match_string(It &s, const It &e, const std::string &str)
{
	for (char c : str) {
		if (s == e || *s != c) {
			throw parser_error{str};
		}

		s++;
	}
}

template <typename It>
void space(It &s, const It &e)
{
	if (s == e || !std::isspace(*s)) {
		throw parser_error{"whitespace"};
	}

	do {
		s++;
	} while (s != e && std::isspace(*s));
}

template <typename It>
void maybe_space(It &s, const It &e)
{
	try {
		space(s, e);
	} catch (const parser_error &) {

	}
}

template <typename It>
void equals(It &s, const It &e)
{
	maybe_space(s, e);
	match_char(s, e, '=');
	maybe_space(s, e);
}

template <typename It>
std::string version_info(It &s, const It &e)
{
	space(s, e);
	match_string(s, e, "version");
	equals(s, e);

	auto curr = s;

	try {
		match_string(curr, e, "'1.0'");
	} catch (const parser_error &) {
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
	maybe_space(s, e);
	match_string(s, e, last_match);

	return version;
}

template <typename It>
xml_doc prologue(It &s, const It &e)
{
	xml_doc doc;

	doc.version = xml_decl(s, e);

	maybe_space(s, e);

	return doc;
}

template <typename It>
std::string name(It &s, const It &e)
{
	std::string ret;
	char c;

	if (s == e) {
		throw parser_error{"name"};
	}

	c = *s;
	if (!std::isalpha(c) && c != '_' && c != ':') {
		throw parser_error{"name"};
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
		throw parser_error{"chardata"};
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
	} catch (const parser_error &) {
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
				space(s, e);
				ret->attributes.insert(attribute(s, e));
			} catch (const parser_error &) {
				break;
			}
		}

		try {
			auto curr = s;

			match_char(curr, e, '/');
			empty_node = true;

			s = curr;
		} catch (const parser_error &) {
			empty_node = false;
		}

		match_char(s, e, '>');

		if (empty_node) {
			return ret;
		}

		while (true) {
			maybe_space(s, e);

			try {
				auto curr = s;

				ret->children.push_back(element(curr, e));

				s = curr;
			} catch (const parser_error &) {
				break;
			}
		}

		if (ret->children.empty()) {
			try {
				ret->value = chardata(s, e);
				rtrim(ret->value);
			} catch (const parser_error &) {

			}
		}

		match_char(s, e, '<');
		match_char(s, e, '/');
		match_string(s, e, ret->name);
		maybe_space(s, e);
		match_char(s, e, '>');
	} catch (const parser_error &) {
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
		auto curr = s;

		ret = prologue(curr, e);

		s = curr;
	} catch (const parser_error &ex) {
		ret.version = "1.0";
	}

	ret.root = element(s, e);

	return ret;
}

}
}
