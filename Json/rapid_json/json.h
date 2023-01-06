#ifndef JSON_H_
#define JSON_H_

#define RAPIDJSON_HAS_STDSTRING 1

#include <string>
#include "string.h"
#include "document.h"


class Json {
public:
	using Document = rapidjson::Document;
	using SizeType = rapidjson::SizeType;
	using Value = rapidjson::Value;

	Json() {}
	~Json() {}

	/**
	 * Read a json file.
	 * Read from the file specified in the parameter.
	 * @param file a json file included full path
	 * @return If succeeded return true, otherwise false
	 */
	bool read(std::string const& file);

	/**
	 * Write a json file.
	 * Write to the file specified in the parameter.
	 * @param file a json file included full path
	 * @return If succeeded return true, otherwise false
	 */
	bool write(std::string const& file);

	Document& document() { return document_; }

private:
	FILE* fp_ = nullptr;
	rapidjson::Document document_;
};

#endif