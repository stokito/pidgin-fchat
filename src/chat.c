#include "fchat.h"

GHashTable *fchat_prpl_chat_info_defaults(PurpleConnection *gc, const gchar *room) {
	GHashTable *defaults;
	defaults = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_free);
	g_hash_table_insert(defaults, "room", g_strdup(FCHAT_CHAT_ROOM_NAME));
	return defaults;
}

static void joined_chat(gpointer key, gpointer value, gpointer user_data) {
	PurpleConvChat *chat = (PurpleConvChat *)user_data;
	purple_conv_chat_add_user(chat, (gchar *)key, NULL, PURPLE_CBFLAGS_NONE, FALSE);
}

void fchat_prpl_chat_join(PurpleConnection *gc, GHashTable *components) {
	FChatConnection *fchat_conn = (FChatConnection *)gc->proto_data;
	const gchar *room = g_hash_table_lookup(components, "room");
	if (strcmp(room, FCHAT_CHAT_ROOM_NAME) != 0) {
		purple_debug_error("fchat", "Only '%s' chat room can be used in fchat\n", FCHAT_CHAT_ROOM_NAME);
		return;
	}

	PurpleConversation *conv = purple_find_chat(gc, FCHAT_CHAT_ROOM_ID);
	if (!conv) {
		conv = serv_got_joined_chat(gc, FCHAT_CHAT_ROOM_ID, room);
		PurpleConvChat *chat = purple_conversation_get_chat_data(conv);
		g_hash_table_foreach(fchat_conn->buddies, joined_chat, chat);
	} else {
		purple_debug_info("fchat", "already in chat room %s\n", room);
	}
}

gchar *fchat_prpl_chat_get_name(GHashTable *components) {
	const gchar *room = g_hash_table_lookup(components, "room");
	return g_strdup(room);
}

GList *fchat_prpl_chat_info(PurpleConnection *gc) {
	/* returning chat setting 'room' */
	struct proto_chat_entry *pce;
	pce = g_new0(struct proto_chat_entry, 1);
	pce->label = _("Chat _room");
	pce->identifier = "room";
	pce->required = TRUE;
	return g_list_append(NULL, pce);
}

PurpleRoomlist *fchat_prpl_chat_get_roomlist(PurpleConnection *gc) {
	PurpleRoomlist *roomlist = purple_roomlist_new(gc->account);
	GList *fields = NULL;
	PurpleRoomlistField *field;

	/* set up the room list */
	field = purple_roomlist_field_new(PURPLE_ROOMLIST_FIELD_STRING, _("room"),  "room", TRUE /* hidden */);
	fields = g_list_append(fields, field);
	purple_roomlist_set_fields(roomlist, fields);

	PurpleRoomlistRoom * room = purple_roomlist_room_new(PURPLE_ROOMLIST_ROOMTYPE_ROOM, FCHAT_CHAT_ROOM_NAME, NULL);
	purple_roomlist_room_add(roomlist, room);
	return roomlist;
}

typedef struct {
	FChatConnection *fchat_conn;
	const gchar *message;
} SendPublicChatMsgData;

static void send_public_chat_msg(gpointer key, gpointer value, gpointer user_data) {
	SendPublicChatMsgData *data = (SendPublicChatMsgData *)user_data;
	gboolean msg_confirmation = purple_account_get_bool(data->fchat_conn->gc->account, "confirm_public_msg", FALSE);
	fchat_send_msg_cmd(data->fchat_conn, (FChatBuddy *)value, data->message, FCHAT_MSG_TYPE_PUBLIC, msg_confirmation);
}

gint fchat_prpl_chat_send(PurpleConnection *gc, gint id, const gchar *message, PurpleMessageFlags flags) {
	PurpleConversation *conv = purple_find_chat(gc, id);
	if (!conv) {
		purple_debug_error("fchat", "tried to send message to chat room #%d: %s\n but couldn't find chat room", id, message);
		return -1;
	}
	// send message to everyone in the chat room
	SendPublicChatMsgData *data = g_new(SendPublicChatMsgData, 1);
	data->fchat_conn = (FChatConnection *)gc->proto_data;
	data->message = message;
	g_hash_table_foreach(data->fchat_conn->buddies, send_public_chat_msg, data);
	PurpleConvChat *chat = purple_conversation_get_chat_data(conv);
	purple_conv_chat_write(chat, gc->account->username, message, PURPLE_MESSAGE_SEND, time(NULL));
	g_free(data);
	return 0;
}
