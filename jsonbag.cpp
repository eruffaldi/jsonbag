/**
 * JSONBAG
 */
#include "jsonbag.hpp"
#include "base64.h"
#include <sstream>
#include <fstream>
#include "mongoose.h"

template <class F>
void streamcopyfix(std::istream & inf, int left, F f)
{
  uint8_t buf[32*1024];
  while(inf && left > 0)
  {
    inf.read((char*)buf,left > sizeof(buf) ? sizeof(buf) : left);
    int q = inf.gcount();
    if(q <= 0)
      break;
    f(buf,q);
    left -= q;
  }
  while(left > 0)
  {
    int k = left > sizeof(buf) ? sizeof(buf) : left;
    memset(buf,0,k);
    f(buf,k);
    left -= k;            
  }  
}

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
      int firstlinesize = sprintf(bufout,"JSOB00%X\r\n",(int)s.size());
      int sepsize = 3; // CRLF\x00
      int totsize = size() + s.size() + firstlinesize + sepsize;
      char headers[256];
      sprintf(headers, "Content-Type: application/jsonbag\r\nJSONBag-Range: %d %d",firstlinesize,(int)s.size());
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
          std::ifstream inf(b.filename,std::ios::binary);
          streamcopyfix(inf,b.size,[nc] (uint8_t*p,int n) {     mg_send(nc,p,n); });
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
          std::ifstream inf(b.filename,std::ios::binary);
          streamcopyfix(inf,b.size,[&ons] (uint8_t*p,int n) {     ons.write((char*)p,n); });
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
    // OPTIONAL: if mongoos is present we could use mg_base64_encode
    // TODO: not load all but make it incremental
    cs_stat_t ss;
    mg_stat(path.c_str(),&ss);

    std::vector<uint8_t> data(ss.st_size);
    std::ifstream inf(filename,std::ios::binary);
    inf.read((char*)data.size(),data.size());

    e = "data:" + mime +  ";base64," + base64_encode(data.data(),data.size());
  }
  else {
    if(deferload)
    {
      // load in block
      blocks.push_back(BinaryBlock());
      auto & b = blocks.back();
      cs_stat_t ss;
      mg_stat(b.path.c_str(),&ss);
      b.size = ss.st_size;
      b.filename = filename;
      b.path = path;
      b.mime = mime;

      // common
      b.buildPrefix();
      b.offset = currentoff + b.prefix.size();
      e = b.buildUrl();
      currentoff += b.effectiveSize();
    }
    else
    {
      // load file as binary       // TODO add block
      blocks.push_back(BinaryBlock());
      auto & b = blocks.back();
      cs_stat_t ss;
      mg_stat(b.path.c_str(),&ss);
      b.size = ss.st_size;
      b.path = path;
      b.mime = mime; 
      auto pp = new uint8_t[b.size];
      b.managed = std::shared_ptr<const uint8_t>(pp,[] (uint8_t*p) { delete [] p;});
      std::ifstream inf(filename,std::ios::binary);
      inf.read((char*)pp,b.size);

      // common
      b.buildPrefix();
      b.offset = currentoff + b.prefix.size();
      e = b.buildUrl();
      currentoff += b.effectiveSize();
    }
  }
    return 0;
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
      BinaryBlock b;
      b.size = size;
      b.mime = mime;
      b.path = path;
      b.managed = m;
      b.buildPrefix();
      b.offset = currentoff + b.prefix.size();
      value = b.buildUrl();
      currentoff += b.effectiveSize();
      blocks.push_back(b);
  }
    return 0;
}

void JSONBagBuilder::BinaryBlock::buildPrefix()
{
    int headersize = 4 + 2 + path.size() + 2 + mime.size();

    std::ostringstream ons; // this holds the prefix of everyblob
    uint32_t size32 = size;
    ons.write((char*)&size32,4); // TODO endianess safety

    uint16_t size16 = path.size();
    ons.write((char*)&size16,2); // TODO endianess safety
    ons.write(path.c_str(),path.size());

    size16 = mime.size();
    ons.write((char*)&size16,2); // TODO endianess safety
    ons.write(mime.c_str(),mime.size());

    prefix = ons.str();
}

std::string JSONBagBuilder::BinaryBlock::buildUrl()
{
  std::ostringstream onslink;
  onslink << "linkdata:" << mime << ";offset=" << offset<<";size=" << size; // not standard but reasonable
  return onslink.str();
}
