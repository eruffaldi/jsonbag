#pragma once

#include "json/json.h"
#include <string>
#include <iostream>

class JSONBinBuilder
{
public:
	struct BinaryBlocks
	{
		int size;
		int offset;	// offset of payload
		std::string prefix; // built prefix
		std::shared_ptr<const uint8_t> managed; 
		std::string filename;

		bool isFile() const { return filename.size(); }
		int effectiveSize() const { return size+prefis.size(); }
	};

	void setInlineMode(bool b) { usebase64 = b; } 

	// TODO: optiion for sending out chunked
	void serialize(struct mg_connection * nc);

	// writes to iostream
	void serialize(std::ostream & ons);

	// reads from iostream
	// void deserialize(std::istream & ins);

	int assignBinary(Json::Value & , std::string path, std::string mime, std::shared_ptr<const uint8_t> m , int size);

	int assignFile(Json::Value & , std::string path, std::string mime, std::string filename, bool deferload = false);

	Json::Value root;

private:
	int size() const { return currentoff; }
	int currentoff = 0;
	std::vector<BinaryBlocks> blocks;
	bool usebase64 = false;
};

// TODO wrappers to : std::shared_ptr<const uint8_t>