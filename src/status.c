#include "fchat.h"

gchar *fchat_prpl_status_text(PurpleBuddy *buddy) {
	const gchar *message;
	if (purple_find_buddy(buddy->account, buddy->name)) {
		PurplePresence *presence = purple_buddy_get_presence(buddy);
		PurpleStatus *status = purple_presence_get_active_status(presence);
		const gchar *name = purple_status_get_name(status);
		message = purple_status_get_attr_string(status, "message");
	} else {
		message = "";
	}
	return g_strdup(message);
}

GList *fchat_prpl_status_types(PurpleAccount *account) {
	GList *types = NULL;
	PurpleStatusType *type;

	type = purple_status_type_new(PURPLE_STATUS_AVAILABLE, FCHAT_STATUS_ONLINE, FCHAT_STATUS_ONLINE, TRUE);
	types = g_list_prepend(types, type);

	type = purple_status_type_new(PURPLE_STATUS_AWAY, FCHAT_STATUS_AWAY,  FCHAT_STATUS_AWAY, TRUE);
	purple_status_type_add_attr(type, "message", _("Message"), purple_value_new(PURPLE_TYPE_STRING));
	types = g_list_prepend(types, type);

	type = purple_status_type_new(PURPLE_STATUS_OFFLINE, NULL, NULL, TRUE);
	types = g_list_prepend(types, type);

	return g_list_reverse(types);
}

typedef struct {
	FChatConnection *fchat_conn;
	PurpleStatus *status;
} StatusFuncData;

void func_send_status(gpointer key, gpointer value, gpointer user_data) {
	StatusFuncData *status_func_data = (StatusFuncData *)user_data;
	fchat_send_status_cmd(status_func_data->fchat_conn, (FChatBuddy *)value, status_func_data->status);
}

void fchat_prpl_set_status(PurpleAccount *account, PurpleStatus *status) {
	const gchar *msg = purple_status_get_attr_string(status, "message");
	StatusFuncData *status_func_data = g_new0(StatusFuncData, 1);
	status_func_data->fchat_conn = (FChatConnection *)account->gc->proto_data;
	status_func_data->status = status;
	g_hash_table_foreach(status_func_data->fchat_conn->buddies, func_send_status, status_func_data);
	g_free(status_func_data);
}
