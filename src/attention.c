#include "fchat.h"

gboolean fchat_prpl_send_attention(PurpleConnection *gc, const gchar *username, guint type) {
	g_return_if_fail(type == 0);
	FChatConnection *fchat_conn = (FChatConnection *)gc->proto_data;
	FChatBuddy *fchat_buddy = fchat_find_buddy(fchat_conn, username, FALSE);
	fchat_send_beep_cmd(fchat_conn, fchat_buddy);
}

GList *fchat_prpl_get_attention_types(PurpleAccount *account) {
	static GList *list = NULL;
	if (!list) {
		list = g_list_append(list, purple_attention_type_new("Nudge", _("Nudge"),
			_("%s has nudged you!"), _("Nudging %s...")));
	}
	return list;
}
