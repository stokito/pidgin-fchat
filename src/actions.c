#include "fchat.h"

static void set_board_cb(PurpleAccount *account, const gchar *msg) {
	FChatConnection *fchat_conn = (FChatConnection *)account->gc->proto_data;
	if (fchat_conn->my_buddy->msg_board) {
		g_free(fchat_conn->my_buddy->msg_board);
	}
	fchat_conn->my_buddy->msg_board = g_strdup(msg);
	purple_account_set_string(account, "msg_board", msg);
}

static void fchat_action_set_msg_board(PurplePluginAction *action) {
	PurpleConnection *gc = (PurpleConnection *)action->context;
	PurpleAccount *account = purple_connection_get_account(gc);
	const gchar *msg = purple_account_get_string(account, "msg_board", NULL);
	purple_request_input(gc, _("Set board"), _("Set message board text"), NULL,
		msg,
		TRUE, FALSE,
		gc && (gc->flags & PURPLE_CONNECTION_HTML) ? "html" : NULL,
		_("Save"), G_CALLBACK(set_board_cb), _("Cancel"), NULL,
		account, NULL, NULL, account
	);
}


GList *fchat_prpl_actions(PurplePlugin *plugin, gpointer context) {
	GList *actions = NULL;
	PurplePluginAction *action;
	action = purple_plugin_action_new(
			_("Set user info..."), fchat_action_set_user_info);
	actions = g_list_append(actions, action);
	action = purple_plugin_action_new(
			_("Set message board..."), fchat_action_set_msg_board);
	actions = g_list_append(actions, action);
	return actions;
}

static void blist_action_beep(PurpleBlistNode *node, gpointer userdata) {
	PurpleBuddy *purple_buddy = (PurpleBuddy *)node;
	purple_prpl_send_attention(purple_buddy->account->gc, purple_buddy->name, 0);
}

static void blist_action_get_msg_board(PurpleBlistNode *node, gpointer userdata) {
	PurpleBuddy *purple_buddy = (PurpleBuddy *)node;
	FChatConnection *fchat_conn = (FChatConnection *)purple_buddy->account->gc->proto_data;
	FChatBuddy *fchat_buddy = fchat_find_buddy(fchat_conn, purple_buddy->name, NULL, FALSE);
	fchat_send_get_msg_board_cmd(fchat_conn, fchat_buddy);
}

static void blist_action_get_buddies(PurpleBlistNode *node, gpointer userdata) {
	PurpleBuddy *purple_buddy = (PurpleBuddy *)node;
	FChatConnection *fchat_conn = (FChatConnection *)purple_buddy->account->gc->proto_data;
	FChatBuddy *fchat_buddy = fchat_find_buddy(fchat_conn, purple_buddy->name, NULL, FALSE);
	fchat_send_get_buddies(fchat_conn, fchat_buddy);
}

GList *fchat_prpl_blist_node_menu(PurpleBlistNode *node) {
	if (!PURPLE_BLIST_NODE_IS_BUDDY(node)) {
		return NULL;
	}
	GList *actions = NULL;
	PurpleMenuAction *action;
	action = purple_menu_action_new(
		_("Send Beep"),
		PURPLE_CALLBACK(blist_action_beep),
		NULL,   /* userdata passed to the callback */
		NULL);  /* child menu items */
	actions = g_list_append(actions, action);

	action = purple_menu_action_new(
		_("Get board message..."),
		PURPLE_CALLBACK(blist_action_get_msg_board),
		NULL,   /* userdata passed to the callback */
		NULL);  /* child menu items */
	actions = g_list_append(actions, action);

	action = purple_menu_action_new(
		_("Get buddies..."),
		PURPLE_CALLBACK(blist_action_get_buddies),
		NULL,   /* userdata passed to the callback */
		NULL);  /* child menu items */
	actions = g_list_append(actions, action);

	return actions;
}
