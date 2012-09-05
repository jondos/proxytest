#include "packet-anon.hpp"

static gboolean anon_server_already_seen(address clientadr) {
	GHashTableIter iter;
	gpointer serveradr, sd;
	g_hash_table_iter_init (&iter, anon_server);
	while (g_hash_table_iter_next (&iter, &serveradr, &sd)) {
		if (!CMP_ADDRESS((address*)serveradr, &clientadr)) {
			return 1;
		}
	}
	return 0;	
}

static gboolean cmp_address(const void *addr1, const void *addr2) {
	return CMP_ADDRESS((address*)addr1, (address*)addr2) == 0 ? TRUE : FALSE;
}

static void range_delete_anon_tcp_callback(guint32 port) {
	dissector_delete_uint("tcp.port", port, anon_handle);
}

static void range_add_anon_tcp_callback(guint32 port) {
	dissector_add_uint("tcp.port", port, anon_handle);
}

static void apply_anon_prefs(void) {
	range_foreach(anon_tcp_range, range_delete_anon_tcp_callback);
	g_free(anon_tcp_range);
	anon_tcp_range = range_copy(global_anon_tcp_range);
	range_foreach(anon_tcp_range, range_add_anon_tcp_callback);
}


/* determine PDU length of protocol anon */
static guint get_anon_initmessage_len(packet_info *pinfo, tvbuff_t *tvb, int offset)
{
	(void) pinfo; /* avoid unused-parameter warning */
	return (guint)(tvb_get_ntohs(tvb, offset)) + 2;
}

static void dissect_anon_initmessage(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	tvbuff_t *xml_tvb;
	address  *tmpaddr;
	serverdata *sd;

	xml_tvb = tvb_new_subset(tvb, 2, tvb_length_remaining(tvb, 2), tvb_reported_length_remaining(tvb, 2));
	call_dissector(xml_handle, xml_tvb, pinfo, NULL);

	/* check if it is a server which we didn't see yet */
	if ((0 == g_strcmp0(((xml_frame_t*)pinfo->private_data)->last_child->name_orig_case,"MixCascade"))
			&& !anon_server_already_seen(pinfo->src)) {
			tmpaddr = g_new(address,1);
			MY_COPY_ADDRESS(tmpaddr,&(pinfo->src));

			sd =  g_new(serverdata,1);
			sd->mix_protocol_version = 0.4;
			sd->mix_count = 2;

			g_hash_table_insert(anon_server,tmpaddr, sd);
	}

	col_set_str(pinfo->cinfo, COL_PROTOCOL, "AN.ON");
	/* Clear out stuff in the info column */
	col_clear(pinfo->cinfo,COL_INFO);
	col_add_fstr(pinfo->cinfo, COL_INFO, "initialization message");
	if (tree) { /* we are being asked for details */
		proto_item *ti = NULL;
		proto_tree *anon_tree = NULL;

		ti = proto_tree_add_item(tree, proto_anon, tvb, 0, -1, ENC_NA);
		proto_item_append_text(ti, ", initialization message");

		anon_tree = proto_item_add_subtree(ti, ett_anon);
		proto_tree_add_item(anon_tree, hf_anon_packet_length, tvb, 0, 2, ENC_BIG_ENDIAN);

		call_dissector(xml_handle, xml_tvb, pinfo, anon_tree);
	}
}

static void dissect_anon(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	/* check if it is one of the 3 unencrypted xml packets
	 * check if packet starts with <?xml ("<?xm" == 0x3C3F786D)*/
	if (0x3C3F786D == tvb_get_ntohl(tvb, 2)) {
		tcp_dissect_pdus(tvb, pinfo, tree, TRUE, ANON_FRAME_HEADER_LEN,
				get_anon_initmessage_len, dissect_anon_initmessage);
	} else {
		/* assume all communication from and to already seen server is AN.ON */
		if (anon_server_already_seen(pinfo->src) || anon_server_already_seen(pinfo->dst)) {
			col_set_str(pinfo->cinfo, COL_PROTOCOL, "AN.ON");
			/* Clear out stuff in the info column */
			col_clear(pinfo->cinfo,COL_INFO);
			col_add_fstr(pinfo->cinfo, COL_INFO, "encrypted message");

			if (tree) { /* we are being asked for details */
				proto_item *ti = NULL;
				/*proto_tree *anon_tree = NULL;*/

				ti = proto_tree_add_item(tree, proto_anon, tvb, 0, -1, ENC_NA);
				proto_item_append_text(ti, ", encrypted message");
				/*anon_tree = proto_item_add_subtree(ti, ett_anon);*/
				/*proto_tree_add_item(anon_tree, hf_anon_packet_type, tvb, 0, 0, ENC_BIG_ENDIAN);*/
			}
		}
	}
}



void proto_register_anon(void)
{
	module_t* anon_module;
	static hf_register_info hf[] = {
		{ &hf_anon_packet_length,
			{ "AN.ON packet length", "anon.packet_length",
				FT_UINT8, BASE_DEC,
				NULL, 0x0,
				NULL, HFILL },
		}
	};

	/* Setup protocol subtree array */
	static gint *ett[] = {
		&ett_anon
	};

	proto_anon = proto_register_protocol (
			"AN.ON Protocol", /* name       */
			"AN.ON",      /* short name */
			"anon"       /* abbrev     */
			);

	anon_module = prefs_register_protocol(proto_anon,apply_anon_prefs);
	prefs_register_range_preference(anon_module, "tcp.port", "TCP Ports",
			"TCP Ports range",
			&global_anon_tcp_range, 65535);

	proto_register_field_array(proto_anon, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));

	anon_server = g_hash_table_new_full(g_direct_hash, cmp_address, g_free, g_free);
}


void proto_reg_handoff_anon(void)
{
	anon_handle = create_dissector_handle(dissect_anon, proto_anon);
	xml_handle = find_dissector("xml");
}

