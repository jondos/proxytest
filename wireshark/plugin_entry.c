#include "packet-anon.hpp"
#ifndef ENABLE_STATIC

G_MODULE_EXPORT const gchar version[] = "0.0.1";

G_MODULE_EXPORT void plugin_register (void)
{
	proto_register_anon ();
}

G_MODULE_EXPORT void plugin_reg_handoff(void)
{
	proto_reg_handoff_anon ();
}
#endif