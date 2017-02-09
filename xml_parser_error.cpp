#include "xml_parser_error.h"

using namespace xml;

parser_error::parser_error(const std::string &msg) :
	err_string{msg}
{

}

const char *parser_error::what() const noexcept
{
	return error_string().c_str();
}

const std::string parser_error::error_string() const
{
	return "token not found: " + err_string;
}
