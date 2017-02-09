#pragma once

#include <stdexcept>
#include <string>

namespace xml
{

class parser_error : public std::exception
{
public:
	parser_error(const std::string &msg);

	virtual const char *what() const noexcept override;

	virtual const std::string error_string() const;

private:
	std::string err_string;
};

}
