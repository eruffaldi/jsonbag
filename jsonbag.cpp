#include "jsonbag.hpp"
#include "base64.h"
#include <sstream>
#include "mongoose.h"

void JSONBagBuilder::serialize(struct mg_connection * nc)
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
      char bufout[128];
      int firstlinesize = sprintf(bufout,"JSOB00%X\r\n",s.size());
      int sepsize = 3; // CRLF\x00
      int totsize = size() + s.size() + firstlinesize + sepsize;
      char headers[256];
      sprintf(headers, "Content-Type: application/jsonbag\r\nJSONBag-Range: %d-%d",firstlinesize,firstlinesize+s.size());
      mg_send_head(nc, 200, totsize, headers);
      mg_send(nc, s.c_str(),s.size());
      // separator
      mg_send(nc, "\r\n\x00",3);
      // body
      for(auto & b : blocks)
      {
        mg_send(nc, b.prefix.c_str(),b.prefix.size());
        if(b.isFile())
        {
          // NOTE: under Linux mongoose should expose sendfile
          std::ifstream inf(b.filename);
          char buf[32*1024];
          int left = b.size;
          while(inf && left > 0)
          {
            int q = inf.read(buf,left > sizeof(buf) ? sizeof(buf) : left);
            if(q <= 0)
              break;
            mg_send(nc,buf,q);
            left -= q;
          }
          while(left > 0)
          {
            int k = left > sizeof(buf) ? sizeof(buf) : left;
            memset(buf,0,k);
            mg_send(nc,buf,k);
            left -= k;            
          }
        }
        else
        {
          mg_send(nc, b.managed.get(), b.size);
        }
      }
    }
}

// writes to iostream AKA file
void JSONBagBuilder::serialize(std::ostream & ons)
{
    Json::StyledWriter styledWriter;
    std::string s = styledWriter.write(root);
    if(usebase64)
    {
      ons << s;
    }
    else
    {
      // TODO emit size before JSON
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

int JSONBagBuilder::assignFile(Json::Value & e , std::string path, std::string mime, std::string filename, bool deferload)
{
  if(usebase64)
  {
    // load in anycase as 64bit no defer allowed: except external URL
     

    //assignFile(e,path,mime,filename,false); // load NOW
  }
  else {
    if(deferload)
    {
      // load in block
      blocks.push_back(BinaryBlock())
      auto & b = bloks.back();
      cs_stat_t ss;
      mg_stat(b.path.c_str(),&ss);
      b.size = ss.st_size;
      b.offset = size();
      b.filename = filename;
      b.path = path;
      b.mime = mime;
    }
    else
    {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              ù
      // load file as binary       // TODO add block
    }
  }
}

// reads from iostream
// void deserialize(std::istream & ins);

int JSONBagBuilder::assignBinary(Json::Value & value , std::string path, std::string mime, std::shared_ptr<const uint8_t> m , int size)
{
  if(usebase64)
  {
      // OPTIONAL: if mongoos is present we could use mg_base64_encode
      value = "data:" + mime +  ";base64," + base64_encode(m.get(),size);
      return 0;
  }
  // TODO: add the external mode
  else
  {
      int headersize = 4 + 2 + path.size() + 2 + mime.size();
      currentoff += headersize;
      int r = currentoff;

      std::ostringstream ons; // this holds the prefix of everyblob
      uint32_t size32 = size;
      ons.write((char*)&size32,4); // TODO endianess safety

      uint16_t size16 = path.size();
      ons.write((char*)&size16,2); // TODO endianess safety
      ons.write(path.c_str(),path.size());

      size16 = mime.size();
      ons.write((char*)&size16,2); // TODO endianess safety
      ons.write(mime.c_str(),mime.size());

      BagaryBlock bb;
      bb.prefix = ons.str();
      bb.size = size;
      bb.offset = currentoff;

      std::ostringstream onslink;
      onslink << "data:" << mime << ";offset=" << currentoff<<";size=" << size; // not standard but reasonable
      value = onslink.str();

      currentoff += size;


      blocks.push_back(bb);
  }
}
