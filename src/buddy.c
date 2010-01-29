#include "fchat.h"

FChatBuddy *fchat_buddy_new(const gchar *host, gint port) {
	FChatBuddy *buddy = g_new0(FChatBuddy, 1);
	buddy->host = g_strdup(host);
	buddy->addr = gnet_inetaddr_new(host, port);
	if (buddy->addr == NULL) {
		purple_debug_error("fchat", "Incorrect address %s:%d\n", host, port);
	}
	return buddy;
}

FChatBuddy *fchat_find_buddy(FChatConnection *fchat_conn, const gchar *host, gboolean create_if_not_exists) {
	FChatBuddy *buddy = NULL;
	buddy = g_hash_table_lookup(fchat_conn->buddies, host);
	if ((buddy == NULL) && create_if_not_exists) {
		gint port = purple_account_get_int(fchat_conn->gc->account, "port", FCHAT_DEFAULT_PORT);
		buddy = fchat_buddy_new(host, port);
		g_hash_table_insert(fchat_conn->buddies, g_strdup(host), buddy);
	}
	return buddy;
}

void fchat_buddy_delete(gpointer p) {
	FChatBuddy *buddy = p;
	if (buddy->host != NULL)
		g_free(buddy->host);
	if (buddy->alias != NULL)
		g_free(buddy->alias);
	if (buddy->info != NULL)
		fchat_buddy_info_destroy(buddy->info);
	if (buddy->addr != NULL)
		gnet_inetaddr_delete(buddy->addr);
	g_free(buddy);
}

void fchat_request_authorization_allow_cb(void *user_data) {
	FChatRequestAuthorizationCbData *cb_data = user_data;
	fchat_send_connect_confirm_cmd(cb_data->fchat_conn, cb_data->buddy, TRUE);
	g_free(cb_data);
}

void fchat_request_authorization_deny_cb(void *user_data) {
	FChatRequestAuthorizationCbData *cb_data = user_data;
	fchat_send_connect_confirm_cmd(cb_data->fchat_conn, cb_data->buddy, FALSE);
	g_free(cb_data);
}

void fchat_load_buddy_list(FChatConnection *fchat_conn) {
	gint port = purple_account_get_int(fchat_conn->gc->account, "port", FCHAT_DEFAULT_PORT);
	GSList *buddies, *l;
	buddies = purple_find_buddies(fchat_conn->gc->account, NULL);
	for(l = buddies; l; l = l->next) {
		PurpleBuddy *buddy = (PurpleBuddy *)l->data;
		FChatBuddy *fchat_buddy = fchat_buddy_new(buddy->name, port);
		g_hash_table_insert(fchat_conn->buddies, g_strdup(buddy->name), fchat_buddy);
	}
	g_slist_free(buddies);
}

void fchat_prpl_add_buddy(PurpleConnection *gc, PurpleBuddy *buddy, PurpleGroup *group) {
	FChatConnection *fchat_conn = gc->proto_data;
	FChatBuddy *fchat_buddy = fchat_find_buddy(fchat_conn, buddy->name, TRUE);
	fchat_send_connect_cmd(fchat_conn, fchat_buddy);
}

void fchat_prpl_remove_buddy(PurpleConnection *gc, PurpleBuddy *buddy, PurpleGroup *group) {
	FChatConnection *fchat_conn = gc->proto_data;
	g_hash_table_remove(fchat_conn->buddies, buddy->name);
}

FChatBuddy **fchat_get_buddies_list_all(FChatConnection *fchat_conn) {
	gint size = g_hash_table_size(fchat_conn->buddies);
	FChatBuddy **buddies_list = g_new(FChatBuddy *, size + 1); /* ... and 1 element for NULL */
	gint i = 0;
	GHashTableIter iter;
	gpointer key;
	FChatBuddy *value;
	g_hash_table_iter_init (&iter, fchat_conn->buddies);
	while (g_hash_table_iter_next(&iter, &key, (gpointer)&value)) {
		buddies_list[i] = value;
		i++;
	}
	buddies_list[i] = NULL;
	return buddies_list;
}

typedef struct {
	FChatConnection *fchat_conn;
	FChatSelectBuddiesListCb *ok_cb;
	FChatSelectBuddiesListCb *cancel_cb;
	void *user_data;
} FChatSelectBuddiesListCbData;

static void fchat_select_buddies_list_ok_cb(FChatSelectBuddiesListCbData *fchat_select_buddies_list_cb_data, PurpleRequestFields *fields) {
	PurpleRequestField *field = purple_request_fields_get_field(fields, "buddies");
	GList *selected_items = purple_request_field_list_get_selected(field);
	FChatBuddy *buddy;
	GHashTable *buddies = g_hash_table_new(g_str_hash, g_str_equal);
	for (; selected_items != NULL; selected_items = g_list_next(selected_items)) {
		buddy = (FChatBuddy *)purple_request_field_list_get_data(field, (gchar *)selected_items->data);
		g_hash_table_insert(buddies, g_strdup(buddy->host), buddy);
	}
	g_list_free(selected_items);

	if (fchat_select_buddies_list_cb_data->ok_cb)
		fchat_select_buddies_list_cb_data->ok_cb(fchat_select_buddies_list_cb_data->fchat_conn, buddies, fchat_select_buddies_list_cb_data->user_data);
	g_hash_table_destroy(buddies);
	g_free(fchat_select_buddies_list_cb_data);
}

static void fchat_select_buddies_list_cancel_cb(FChatSelectBuddiesListCbData *fchat_select_buddies_list_cb_data, PurpleRequestFields *fields) {
	if (fchat_select_buddies_list_cb_data->cancel_cb)
		fchat_select_buddies_list_cb_data->cancel_cb(fchat_select_buddies_list_cb_data->fchat_conn, NULL, fchat_select_buddies_list_cb_data->user_data);
	g_free(fchat_select_buddies_list_cb_data);
}

void fchat_select_buddies_list(FChatConnection *fchat_conn, const gchar *msg_text, GHashTable *buddies, FChatSelectBuddiesListCb *ok_cb, FChatSelectBuddiesListCb *cancel_cb, void *user_data) {
	PurpleRequestFields *fields;
	PurpleRequestFieldGroup *group;
	PurpleRequestField *field;
	fields = purple_request_fields_new();
	group = purple_request_field_group_new(NULL);
	purple_request_fields_add_group(fields, group);

	field = purple_request_field_list_new("buddies", _("Buddies"));
	purple_request_field_list_set_multi_select(field, TRUE);

	GHashTableIter iter;
	gchar *buddy_host;
	FChatBuddy *buddy;
	g_hash_table_iter_init (&iter, buddies);
	while (g_hash_table_iter_next (&iter, (gpointer *)&buddy_host, (gpointer *)&buddy)) {
		gchar *item = g_strdup_printf("%s (%s)", buddy->host, buddy->alias ? buddy->alias : "");
		purple_request_field_list_add(field, item, buddy);
		g_free(item);
		/*	purple_request_field_list_add_selected(field, item, buddy); */
	}
	purple_request_field_group_add_field(group, field);

	FChatSelectBuddiesListCbData *fchat_select_buddies_list_cb_data = g_new(FChatSelectBuddiesListCbData, 1);
	fchat_select_buddies_list_cb_data->fchat_conn = fchat_conn;
	fchat_select_buddies_list_cb_data->ok_cb = ok_cb;
	fchat_select_buddies_list_cb_data->cancel_cb = cancel_cb;
	fchat_select_buddies_list_cb_data->user_data = user_data;

	purple_request_fields(fchat_conn->gc, _("Select buddies"),
		msg_text, _("Use Ctrl or Shift keys for multiply select"), fields,
		_("Ok"), G_CALLBACK(fchat_select_buddies_list_ok_cb),
		_("Cancel"), G_CALLBACK(fchat_select_buddies_list_cancel_cb),
		fchat_conn->gc->account, NULL, NULL,
		fchat_select_buddies_list_cb_data);
}

