
#ifndef __PACKET_ANON_H__
#define __PACKET_ANON_H__
#ifdef _WIN32
	#include "wireshark_config_win32.h"
#else
	#ifdef HAVE_CONFIG_H
		#include "config.h"
	#endif
#endif

#include <glib.h>
#include <gmodule.h>

#ifdef __cplusplus
extern "C"
{
#endif
#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/address.h>
#include <epan/dissectors/packet-tcp.h>
#include <epan/dissectors/packet-xml.h>

/* Start the functions we need for the plugin stuff */

void proto_register_anon (void);
void proto_reg_handoff_anon (void);


#ifdef __cplusplus
}
#endif

static dissector_handle_t anon_handle;
static dissector_handle_t xml_handle; 


static GHashTable* anon_server = NULL;
typedef struct _serverdata {
	double mix_protocol_version;
	int mix_count;
	//GArray *clients;
} serverdata;

static int proto_anon = -1;
static int hf_anon_packet_length = -1;
static gint ett_anon = -1;
static range_t *anon_tcp_range = NULL;
static range_t *global_anon_tcp_range = NULL;
#define ANON_FRAME_HEADER_LEN 2

//extern  void dissect_xml(tvbuff_t*, packet_info*, proto_tree*);


#define MY_COPY_ADDRESS(to, from) { \
	gpointer COPY_ADDRESS_data; \
	(to)->type = (from)->type; \
	(to)->len = (from)->len; \
	COPY_ADDRESS_data = g_malloc((from)->len); \
	memcpy(COPY_ADDRESS_data, (from)->data, (from)->len); \
	(to)->data = COPY_ADDRESS_data; \
	}


#endif /* __PACKET_ANON_H__ */
