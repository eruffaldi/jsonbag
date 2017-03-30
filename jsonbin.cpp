#include "jsonbin.hpp"
#include "base64.h"
#include <sstream>
#include "mongoose.h"

void JSONBinBuilder::serialize(struct mg_connection * nc)
{
    Json::StyledWriter styledWriter; // TODO: non styled
    std::string s = styledWriter.write(root);
    if(usebase64)
    {
      mg_send_head(nc, 200, s.size(), "Content-Type: application/json");
      mg_send(nc, s.c_str(),s.size());
    }
    else
    {
      int firstlinesize = 8+2; // HEX4\xCRLF
      int sepsize = 3; // CRLF\x00
      // emit size
      int totsize = size() + s.size() + firstlinesize + sepsize;
      mg_send_head(nc, 200, s.size(), "Content-Type: application/jsonbin");
      mg_send(nc, s.c_str(),s.size());
      for(auto & b : blocks)
      {
        mg_send(nc, b.prefix.c_str(),b.prefix.size());
        if(b.isFile())
        {
          // load file, but send up to given size! 
        }
        else
        {
          mg_send(nc, b.managed.get(), b.size);
        }
      }
    }
}

// writes to iostream
void JSONBinBuilder::serialize(std::ostream & ons)
{
    Json::StyledWriter styledWriter;
    std::string s = styledWriter.write(root);
    if(usebase64)
    {
      ons << s;
    }
    else
    {
      // emit size
      ons << s;
      for(auto & b : blocks)
      {
        ons << b.prefix;
        if(b.isFile())
        {
          // load file, but send up to given size! 
        }
        else
        {
          ons.write((char*)b.managed.get(), b.size);
        }
      }
    }
}

int JSONBinBuilder::assignFile(Json::Value & e , std::string path, std::string mime, std::string filename, bool deferload)
{
  if(usebase64)
  {
    if(deferload)
      assignFile(e,path,mime,filename,true); // load NOW
    else
    {
      // load and write
    }
  }
  else if(!deferload)
  {
    // load in bloc
  }
  else
  {
    // mark for later
  }
}

// reads from iostream
// void deserialize(std::istream & ins);

int JSONBinBuilder::assignBinary(Json::Value & value , std::string path, std::string mime, std::shared_ptr<const uint8_t> m , int size)
{
  if(usebase64)
  {
      // mg_base64_encode if HAS mongoose
      // allocate size+
      value = "data:" + mime +  ";base64," + base64_encode(m.get(),size);
      return 0;
  }
  else
  {
      int headersize = 4 + 2 + path.size() + 2 + mime.size();
      currentoff += headersize;
      int r = currentoff;
      std::ostringstream ons;
      uint32_t size32 = size;
      ons.write((char*)&size32,4); // TODO endianess

      uint16_t size16 = path.size();
      ons.write((char*)&size16,2); // TODO endianess
      ons.write(path.c_str(),path.size());

      size16 = mime.size();
      ons.write((char*)&size16,2); // TODO endianess
      ons.write(mime.c_str(),mime.size());

      BinaryBlock bb;
      bb.prefix = ons.str();
      bb.size = size;
      bb.offset = currentoff;

      currentoff += size;

      blocks.push_back(bb);
  }
}
