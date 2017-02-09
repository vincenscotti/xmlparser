#pragma once

#include "xml_parser_impl.h"

namespace xml
{

template <typename It>
xml_doc parse(It &s, const It &e)
{
	return parser::document(s, e);
}

}
