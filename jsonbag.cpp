/**
 * JSONBAG Emanuele Ruffaldi 2017
 */
#include "jsonbag.hpp"
#include "base64.h"
#include <sstream>
#include <fstream>

#ifdef WITHMONGOOSE
#include "mongoose.h"
#else
// TODO stat without mongoose
struct cs_stat_t { int st_size; };
void mg_stat(const char * c, cs_stat_t * x) {}
#endif

/**
 Stream Copy with enforcing of padding
 */
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

/** 
 * Mongoose Serialization
 */
void JSONBagBuilder::serialize(struct mg_connection * nc)
{
#ifdef WITHMONGOOSE
    Json::StyledWriter styledWriter; // TODO: non styled
    std::string s = styledWriter.write(root);
    if(isInline())
    {
      mg_send_head(nc, 200, s.size(), "Content-Type: application/json");
      mg_send(nc, s.c_str(),s.size());
    }
    else if(isBag())
    {
      char bufout[128];
      int firstlinesize = sprintf(bufout,"JBAG00%X\r\n",(int)s.size());
      int sepsize = 3; // CRLF\x00
      int totsize = size() + s.size() + firstlinesize + sepsize;
      char headers[256];
      sprintf(headers, "Content-Type: application/jsonbag\r\nJSONBag-Range: %d %d",firstlinesize,(int)s.size());
      mg_send_head(nc, 200, totsize, headers);
      mg_send(nc, bufout,firstlinesize);
      mg_send(nc, s.c_str(),s.size());
      // separator
      mg_send(nc, "\r\n\x00",sepsize);
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
    else
    {
      /**
Content-Length: 538
Content-Type: multipart/related; boundary="e89b3e29388aef23453450d10e5aaed0"


--e89b3e29388aef23453450d10e5aaed0
Content-Type: application/json

--e89b3e29388aef23453450d10e5aaed0
Content-Disposition: attachment; filename="recipe.txt"
Content-Type: text/plain
Content-Length: 86

       */
      char bufout[128];
      int firstlinesize = sprintf(bufout,"JBAG00%X\r\n",(int)s.size());
      int sepsize = 3; // CRLF\x00
      int totsize = size() + s.size() + firstlinesize + sepsize;
      char headers[256];
      sprintf(headers, "Content-Type: application/jsonbag\r\nJSONBag-Range: %d %d",firstlinesize,(int)s.size());
      mg_send_head(nc, 200, totsize, headers);
      mg_send(nc, bufout,firstlinesize);
      mg_send(nc, s.c_str(),s.size());
      // separator
      mg_send(nc, "\r\n\x00",sepsize);
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
#endif
}

/**
 * Serialization to file 
 */
void JSONBagBuilder::serialize(std::ostream & ons)
{
    Json::StyledWriter styledWriter;
    std::string s = styledWriter.write(root);
    if(isInline())
    {
      ons << s;
    }
    else if(isBag())
    {
      char bufout[128];
      int firstlinesize = sprintf(bufout,"JBAG00%X\r\n",(int)s.size());
      int sepsize = 3; // CRLF\x00
      int totsize = size() + s.size() + firstlinesize + sepsize;
      char headers[256];
      ons.write((char*)bufout,firstlinesize);
      ons.write(s.c_str(),s.size());
      ons.write("\r\n\x00",sepsize);

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
    else 
    {
      char bufout[128];
      int firstlinesize = sprintf(bufout,"JBAG00%X\r\n",(int)s.size());
      int sepsize = 3; // CRLF\x00
      int totsize = size() + s.size() + firstlinesize + sepsize;
      char headers[256];
      ons.write((char*)bufout,firstlinesize);
      ons.write(s.c_str(),s.size());
      ons.write("\r\n\x00",sepsize);

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
  if(isInline())
  {

    // OPTIONAL: if mongoos is present we could use mg_base64_encode
    // TODO: not load all but make it incremental
    cs_stat_t ss;
    ss.st_size = 0;
    mg_stat(filename.c_str(),&ss);

    std::cerr << "assignFile " << path << " " << mime << " " << filename << " size " << ss.st_size << std::endl;
    std::vector<uint8_t> data(ss.st_size);
    std::ifstream inf(filename,std::ios::binary);
    inf.read((char*)data.data(),data.size());

    e = "data:" + mime +  ";base64," + base64_encode(data.data(),data.size());
    std::cerr << "assignFile done " << std::endl;
       }
  else {
    if(deferload)
    {
      // load in block
      blocks.push_back(BinaryBlock());
      auto & b = blocks.back();
      cs_stat_t ss;
    ss.st_size = 0;
      mg_stat(filename.c_str(),&ss);
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
    ss.st_size = 0;
      mg_stat(filename.c_str(),&ss);
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
  if(isInline())
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
    Json::Value p;
    p["size"] = size;
    p["mime"] = mime;
    p["path"] = path;
    Json::FastWriter fw;
    auto r = fw.write(p);
    int headersize = 2 + r.size();

    std::ostringstream ons; // this holds the prefix of everyblob
    int n = r.size();
    uint8_t buf[2];
    buf[0] = n & 0xFF;
    buf[1] = (n>> 8) & 0xFF; // LE
    prefix = std::string((char*)buf,2) + r;
}

std::string JSONBagBuilder::BinaryBlock::buildUrl()
{
  std::ostringstream onslink;
  onslink << "linkdata:" << mime << ";offset=" << offset<<";size=" << size; // not standard but reasonable
  return onslink.str();
}

/// the deleter wraps
int JSONBagBuilder::assignBinary(Json::Value & e, std::string path, std::string mime, std::shared_ptr<std::vector<uint8_t> > m)
{
  return assignBinary(e,path,mime,std::shared_ptr<const uint8_t>(m->data(),[m] (const uint8_t *p) mutable { }),m->size());
}


/// the deleter wraps
int JSONBagBuilder::assignBinary(Json::Value & e, std::string path, std::string mime, std::shared_ptr<std::string > m)
{
  return assignBinary(e,path,mime,std::shared_ptr<const uint8_t>((const uint8_t*)m->c_str(),[m] (const uint8_t *p) mutable { }),m->size());
}

void rawmultipart(struct mg_connection * nc, const char * filename)
{
  cs_stat_t ss;
  ss.st_size = 0;
  mg_stat(filename,&ss);

  char headers[256];
  sprintf(headers, "Content-Type: multipart/related");
  mg_send_head(nc, 200, ss.st_size, headers);

  std::ifstream inf(filename,std::ios::binary);
  streamcopyfix(inf,ss.st_size,[nc] (uint8_t*p,int n) {     mg_send(nc,p,n); });
}