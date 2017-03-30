#include "mongoose.h"
#include "jsonbin.hpp"

static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;

void handlejsonbin(struct mg_connection *nc, struct http_message * hm, bool usebin)
{
  mg_send_head(nc, 200, hm->message.len, "Content-Type: text/plain");
  mg_printf(nc, "%.*s", hm->message.len, hm->message.p);

}

static void ev_handler(struct mg_connection *nc, int ev, void *p) {
  struct http_message * hm = (struct http_message *) p;
  if (ev == MG_EV_HTTP_REQUEST) {
    if(mg_vcmp(&hm->uri,"/jsonbin") == 0)
      handlejsonbin(nc,hm,true);
    else if(mg_vcmp(&hm->uri,"/json") == 0)
      handlejsonbin(nc,hm,false);
    else
      mg_serve_http(nc, hm, s_http_server_opts);
  }
}

int main(void) {
  struct mg_mgr mgr;
  struct mg_connection *nc;

  mg_mgr_init(&mgr, NULL);
  printf("Starting web server on port %s\n", s_http_port);
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