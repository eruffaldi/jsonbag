#pragma once

#include "json/json.h"
#include <string>
#include <iostream>

struct mg_connection;

/**
 * Builds a JSON with Bagary attachments:

 - embed binary as data64 links
 - expose them as JSON + blob
 - TODO: expose them as links

 The interesting point is that in any case we refer to things using a link
 */
class JSONBagBuilder
{
public:
	enum class Mode { Base64, Bag, Multipart };

	bool isInline() const { return mode == Mode::Base64; }
	bool isBag() const { return mode == Mode::Bag; }

	Mode mode = Mode::Base64;

	struct BinaryBlock
	{
		int size;
		int offset;	// offset of payload since the beginning of the binary block
		std::string prefix; // built prefix
		std::string path;
		std::string mime;
		std::shared_ptr<const uint8_t> managed; 
		std::string filename;

		void buildPrefix();
		std::string buildUrl();
		bool isFile() const { return filename.size(); }
		int effectiveSize() const { return size+prefix.size(); }
	};

	// TODO: add the external mode
	void setMode(Mode m) { mode = m; }

	// TODO: optiion for sending out chunked data (e.g. per-blob, depending on size)
	void serialize(struct mg_connection * nc);

	// writes to iostream
	void serialize(std::ostream & ons);

	// reads from iostream for reading back
	// void deserialize(std::istream & ins);

	int assignBinary(Json::Value & e, std::string path, std::string mime, std::shared_ptr<std::vector<uint8_t> > m);

	int assignBinary(Json::Value & e, std::string path, std::string mime, std::shared_ptr<std::string> m);
	
	/**
	 The std::shared_ptr<const uint8_t> can hold many types of pointers given the fact that we can use the custom deletere
	 If want to simplify just pass a pointer and let the JSONBagBuilder copy the memory

	 Path is a valid JSONPath that expresses the e variable. If not specified there is not backling
	 */
	int assignBinary(Json::Value & e, std::string path, std::string mime, std::shared_ptr<const uint8_t> m , int size);

	/**
	 * This variant takes a filename. If the deferload is true or we have base64 we'll load the file immediately
		Alternatively we'll load it at serialization time (for JSONBag) or expose it via link (for post streaming)
		The file size should not change
	 */
	int assignFile(Json::Value & e, std::string path, std::string mime, std::string filename, bool deferload = false);

	/// root element
	Json::Value root;

	int size() const { return currentoff; }

private:
	int currentoff = 0;
	std::vector<BinaryBlock> blocks;
	bool usebase64 = false;
};

void rawmultipart(struct mg_connection * nc, const char * filename);

// TODO wrappers to : std::shared_ptr<const uint8_t>