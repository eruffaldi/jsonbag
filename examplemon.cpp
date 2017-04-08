#include "mongoose.h"
#include "jsonbin.hpp"

bool contentNegotiation = false;
static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;

void handlejsonbin(struct mg_connection *nc, struct http_message * hm, bool usebin)
{
  // produce content here
  JSONBinBuilder jb;
  jb.setInlineMode(usebin);
  {
    // create content
    
  }
  jb.serialize(nc);
}

static void ev_handler(struct mg_connection *nc, int ev, void *p) {
  struct http_message * hm = (struct http_message *) p;
  if (ev == MG_EV_HTTP_REQUEST) {
    if(mg_vcmp(&hm->uri,"/jsonbin") == 0)
    {
      bool dobin = true;
      if(contentNegotiation)
      {
        dobin = false;
        for(int i = 0; i < MG_MAX_HTTP_HEADERS; i++)
        {
          if(hm->header_names[i].len == 0)
            break;
          if(mg_vcmp(&hm->header_names[i],"Accept") == 0)
          {
            if(mg_vcmp(&hm->header_values[i],"application/jsonbag") == 0)
            {
              dobin = true;
              break;
            }
          }
        }
      }
      handlejsonbin(nc,hm,dobin);
    }
    else if(mg_vcmp(&hm->uri,"/json") == 0)
      handlejsonbin(nc,hm,false);
    else
      mg_serve_http(nc, hm, s_http_server_opts);
  }
}

int main(int argc, char * argv[]) {
  struct mg_mgr mgr;
  struct mg_connection *nc;

  mg_mgr_init(&mgr, NULL);
  printf("Starting web server on port %s using enforce accept:\n", s_http_port);
  nc = mg_bind(&mgr, s_http_port, ev_handler);
  if (nc == NULL) {
    printf("Failed to create listener\n");
    return 1;
  }

  // Set up HTTP server parameters
  mg_set_protocol_http_websocket(nc);
  s_http_server_opts.document_root = ".";  // Serve current directory
  s_http_server_opts.enable_directory_listing = "yes";

  for (;;) {
    mg_mgr_poll(&mgr, 1000);
  }
  mg_mgr_free(&mgr);

  return 0;
}