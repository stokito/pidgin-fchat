#include "fchat.h"

static void fchat_process_connect_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, FChatPacketBlocks *packet_blocks) {
	/* Определяем версию протокола в пакете */
	if (!purple_account_get_bool(fchat_conn->gc->account, "accept_unknown_protocol_version", TRUE)) {
		gint proto_ver = atoi(packet_blocks->protocol_version);
		if (FCHAT_MY_PROTOCOL_VERSION != proto_ver) {
			/* неизвестный протокол, запрещаем связь */
			purple_debug_info("fchat", "Another protocol version %d from %s, connection can't be established\n", proto_ver, buddy->host);
			fchat_send_connect_confirm_cmd(fchat_conn, buddy, FALSE);
			return;
		}
	}

	/* Если собеседник уже в контакт листе... */
	if (purple_find_buddy(fchat_conn->gc->account, buddy->host)) {
		fchat_send_connect_confirm_cmd(fchat_conn, buddy, TRUE);
		purple_prpl_got_user_status(fchat_conn->gc->account, buddy->host, FCHAT_STATUS_ONLINE, NULL);
	} else if (purple_account_get_bool(fchat_conn->gc->account, "require_authorization", FALSE)) {
		FChatRequestAuthorizationCbData *cb_data = g_new0(FChatRequestAuthorizationCbData, 1);
		cb_data->fchat_conn = fchat_conn;
		cb_data->buddy = buddy;
		purple_account_request_authorization(fchat_conn->gc->account, buddy->host, NULL, buddy->alias,
			NULL, FALSE, fchat_request_authorization_allow_cb, fchat_request_authorization_deny_cb,	cb_data);
	}
}

static void fchat_process_connect_confirm_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, FChatPacketBlocks *packet_blocks) {
	gboolean can_connect = (packet_blocks->msg[0] == 'Y');
	if (can_connect) {
		purple_prpl_got_user_status(fchat_conn->gc->account, buddy->host, FCHAT_STATUS_ONLINE, NULL);
	} else {
		/* TODO */
	}
}

static void fchat_process_ping_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, FChatPacketBlocks *packet_blocks) {
	fchat_send_pong_cmd(fchat_conn, buddy);
}

static void fchat_process_pong_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, FChatPacketBlocks *packet_blocks) {
	purple_prpl_got_user_status(fchat_conn->gc->account, buddy->host, FCHAT_STATUS_ONLINE, NULL);
}

static void fchat_process_disconnect_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, FChatPacketBlocks *packet_blocks) {
	purple_prpl_got_user_status(fchat_conn->gc->account, buddy->host, FCHAT_STATUS_OFFLINE, NULL);
}

static void fchat_process_msg_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, FChatPacketBlocks *packet_blocks) {
	gchar *msg = fchat_decode(fchat_conn, packet_blocks->msg, -1);
	if (msg) {
		serv_got_chat_in(fchat_conn->gc, FCHAT_CHAT_ROOM_ID, buddy->host, PURPLE_MESSAGE_RECV, msg, time(NULL));
		g_free(msg);
	}
	if (packet_blocks->msg_confirmation) {
		fchat_send_confirm_msg_cmd(fchat_conn, buddy, packet_blocks->msg_id);
	}
}

static void fchat_process_private_msg_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, FChatPacketBlocks *packet_blocks) {
	gchar *msg = fchat_decode(fchat_conn, packet_blocks->msg, -1);
	if (msg) {
		serv_got_im(fchat_conn->gc, buddy->host, msg, PURPLE_MESSAGE_RECV, time(NULL));
		g_free(msg);
	}
	if (packet_blocks->msg_confirmation) {
		fchat_send_confirm_msg_cmd(fchat_conn, buddy, packet_blocks->msg_id);
	}
}

static void fchat_process_confirm_msg_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, FChatPacketBlocks *packet_blocks) {
	purple_debug_info("fchat", "fchat_process_confirmation_message not implemented\n");
}

static void fchat_process_delete_msg_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, FChatPacketBlocks *packet_blocks) {
	purple_debug_info("fchat", "fchat_process_delete_message not implemented\n");
}

static void fchat_process_beep_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, FChatPacketBlocks *packet_blocks) {
	purple_prpl_got_attention(fchat_conn->gc, buddy->host, 0);
	purple_sound_play_event(PURPLE_SOUND_POUNCE_DEFAULT,	fchat_conn->gc->account);
	/* TODO FCHAT_BEEP_DENYED_FROM_YOU */
	if (!purple_account_get_bool(fchat_conn->gc->account, "accept_beep", TRUE)) {
		fchat_send_beep_reply_cmd(fchat_conn, buddy, FCHAT_BEEP_DENYED_FROM_ALL);
	} else {
		fchat_send_beep_reply_cmd(fchat_conn, buddy, FCHAT_BEEP_ACEPTED);
	}
}

static void fchat_process_beep_reply_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, FChatPacketBlocks *packet_blocks) {
	const gchar *msg;
	if (!packet_blocks->msg) {
		msg = _("Beep received");             /* BEEP_ACEPTED Собеседник получил сигнал */
	} else if (packet_blocks->msg[0] == '0') {
		msg = _("Signal from you is denied"); /* BEEP_DENYED_FROM_YOU */
	} else { /* packet_blocks->msg[0] == '1' */
		msg = _("Signals are denied");        /* BEEP_DENYED_FROM_ALL */
	}
	serv_got_im(fchat_conn->gc, buddy->host, msg, PURPLE_MESSAGE_SYSTEM, time(NULL));
}

static void fchat_process_status_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, FChatPacketBlocks *packet_blocks) {
	/*
	 * State:Busy;Y[1899-12-30 00:01:59]
	 * State:Busy;N
	 * State:;N
	 * */
	g_return_if_fail(packet_blocks->msg != NULL);
	const gchar *state_begin_pos = packet_blocks->msg + strlen("State:");
	const gchar *state_end_pos = strchr(state_begin_pos, ';');
	g_return_if_fail(state_end_pos != NULL);

	size_t state_len = state_end_pos - state_begin_pos;
	if (state_len == 0)
		return;

	gchar *state = g_strndup(state_begin_pos, state_len);
	gchar state_enabled = *(state_end_pos + 1);

	if (state_enabled == 'Y') {
		const gchar *msg = strstr(state_end_pos + 2, "\r\n");
		gchar *msg_utf8 = msg ? fchat_decode(fchat_conn, msg, -1) : NULL;

		const char *status_id;
		if (g_ascii_strcasecmp(state, FCHAT_STATUS_AWAY) == 0) {
			status_id = FCHAT_STATUS_AWAY;
		} else if (g_ascii_strcasecmp(state, FCHAT_STATUS_BUSY) == 0) {
			status_id = FCHAT_STATUS_BUSY;
		} else if (g_ascii_strcasecmp(state, FCHAT_STATUS_PHONE) == 0) {
			status_id = FCHAT_STATUS_PHONE;
		} else if (g_ascii_strcasecmp(state, FCHAT_STATUS_MUSIC) == 0) {
			status_id = FCHAT_STATUS_MUSIC;
		} else {
			status_id = FCHAT_STATUS_AWAY;
			if (!msg_utf8) { // if no Away message then use the status name as msg
				msg_utf8 = state;
			}
		}
		purple_prpl_got_user_status(fchat_conn->gc->account, buddy->host, status_id, "message", msg_utf8, NULL);
		g_free(msg_utf8);
	} else if (state_enabled == 'N') {
		purple_prpl_got_user_status(fchat_conn->gc->account, buddy->host, FCHAT_STATUS_ONLINE, NULL);
	} else {
		purple_debug_error("fchat", "Unknown status enabled %c\n", state_enabled);
	}
	g_free(state);
}

static void fchat_process_alias_change_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, FChatPacketBlocks *packet_blocks) {
	gchar *alias = fchat_decode(fchat_conn, packet_blocks->msg, -1);
	if (alias) {
		serv_got_alias(fchat_conn->gc, buddy->host, alias);
		g_free(alias);
	}
}

static void fchat_process_get_buddy_info_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, FChatPacketBlocks *packet_blocks) {
	fchat_send_my_buddy_info_cmd(fchat_conn, buddy);
}

static void fchat_process_buddy_info_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, FChatPacketBlocks *packet_blocks) {
	PurpleConnection *gc = fchat_conn->gc;
	FChatBuddyInfo *info = fchat_parse_buddy_info(packet_blocks->msg, fchat_get_connection_codeset(fchat_conn));
	PurpleNotifyUserInfo *purple_info = fchat_buddy_info_to_purple_info(info);
	/* show a buddy's user info in a nice dialog box */
	purple_notify_userinfo(gc, buddy->host,	purple_info, NULL, NULL);
	purple_notify_user_info_destroy(purple_info);
}

static void fchat_process_get_msg_board_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, FChatPacketBlocks *packet_blocks) {
	fchat_send_msg_board_cmd(fchat_conn, buddy);
}

static void fchat_process_msg_board_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, FChatPacketBlocks *packet_blocks) {
	gchar *msg_board = fchat_decode(fchat_conn, packet_blocks->msg, -1);
	if (msg_board) {
		purple_notify_formatted(fchat_conn->gc, _("Board message"), _("Board message"), NULL, msg_board, NULL, NULL);
		g_free(msg_board);
	}
}

static void fchat_select_buddies_list_to_add_cb(FChatConnection *fchat_conn, GHashTable *buddies, void *user_data) {
	PurpleBuddy *purple_buddy;
	GHashTableIter iter;
	gchar *key;
	FChatBuddy *value;
	g_hash_table_iter_init(&iter, buddies);
	while (g_hash_table_iter_next(&iter, (gpointer)&key, (gpointer)&value)) {
		purple_debug_info("fchat", "buddy alias=%s buddy host =%s\n", value->alias, value->host);
		purple_buddy = purple_buddy_new(fchat_conn->gc->account, value->host, value->alias);
		purple_blist_add_buddy(purple_buddy, NULL, NULL, NULL);
		fchat_send_connect_cmd(fchat_conn, (FChatBuddy *)user_data);
	}
}

static void fchat_process_buddy_list_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, FChatPacketBlocks *packet_blocks) {
	purple_debug_info("fchat", "fchat_process_buddy_list not implemented\n");

	GHashTable *buddies = g_hash_table_new(g_str_hash, g_str_equal);
	FChatBuddy *contact_buddy;
	gchar **buddies_v = g_strsplit(packet_blocks->msg, FCHAT_BUDDY_LIST_SEPARATOR, -1);
	gchar *alias;
	gchar *host;
	gchar **p = buddies_v;
	while (*p) {
		alias = *p;
		p++;
		host = *p;
		if (host) {
			contact_buddy = fchat_buddy_new(host, alias);
			g_hash_table_insert(buddies, host, contact_buddy);
			p++;
		}
		purple_debug_info("fchat", "fchat_process_buddy_list_cmd:\nhost=%s alias=%s\n", host, alias);
	}
	g_strfreev(buddies_v);
	fchat_select_buddies_list(fchat_conn, _("Buddy list"), buddies,
		fchat_select_buddies_list_to_add_cb, NULL, buddy);
	g_hash_table_destroy(buddies);
}

static void fchat_select_buddies_list_to_send_cb(FChatConnection *fchat_conn, GHashTable *buddies, void *user_data) {
	fchat_send_buddies(fchat_conn, (FChatBuddy *)user_data, buddies);
}

static void fchat_process_get_buddy_list_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, FChatPacketBlocks *packet_blocks) {
	FChatBuddiesListPrivacy buddy_list_privacy = atoi(purple_account_get_string(fchat_conn->gc->account, "buddy_list_privacy", "0"));
	purple_debug_info("fchat", "buddy_list_privacy = %d\n", buddy_list_privacy);
	if (buddy_list_privacy == FCHAT_BUDDIES_LIST_PRIVACY_REQUEST) {
		fchat_select_buddies_list(fchat_conn, _("%s request your buddy list"), fchat_conn->buddies,
			fchat_select_buddies_list_to_send_cb, fchat_select_buddies_list_to_send_cb, buddy);
	} else if (buddy_list_privacy == FCHAT_BUDDIES_LIST_PRIVACY_ALLOW) {
		fchat_send_buddies(fchat_conn, buddy, fchat_conn->buddies);
	} else { /* FCHAT_BUDDIES_LIST_PRIVACY_DENY */
		fchat_send_buddies(fchat_conn, buddy, NULL);
	}
}

typedef enum {
	BLOCK_PROTOCOL_VERSION = 1,
	BLOCK_NICKNAME         = 2,
	BLOCK_MSG_ID           = 4,
	BLOCK_MSG_CONFIRMATION = 8,
	BLOCK_MSG              = 16
} BlocksToParse;

static struct {
	gchar command;
	BlocksToParse blocks_to_parse; /**< Битовая маска блоки которые нужно извлечь из пакета */
	void (*cb)(FChatConnection *fchat_conn, FChatBuddy *buddy, FChatPacketBlocks *packet_blocks);
} fchat_msgs[] = {
	{ FCHAT_CONNECT_CMD, BLOCK_PROTOCOL_VERSION + BLOCK_NICKNAME, fchat_process_connect_cmd },
	{ FCHAT_CONNECT_CONFIRM_CMD, BLOCK_PROTOCOL_VERSION + BLOCK_NICKNAME + BLOCK_MSG, fchat_process_connect_confirm_cmd },
	{ FCHAT_PING_CMD, 0, fchat_process_ping_cmd },
	{ FCHAT_PONG_CMD, 0, fchat_process_pong_cmd },
	{ FCHAT_DISCONNECT_CMD, 0, fchat_process_disconnect_cmd },

	{ FCHAT_MSG_CMD, BLOCK_MSG_ID + BLOCK_MSG_CONFIRMATION + BLOCK_MSG, fchat_process_msg_cmd },
	{ FCHAT_PRIVATE_MSG_CMD, BLOCK_MSG_ID + BLOCK_MSG_CONFIRMATION + BLOCK_MSG, fchat_process_private_msg_cmd },

	{ FCHAT_CONFIRM_MSG_CMD, BLOCK_MSG_ID, fchat_process_confirm_msg_cmd },
	{ FCHAT_DELETE_MSG_CMD, 0, fchat_process_delete_msg_cmd },

	{ FCHAT_BEEP_CMD, 0, fchat_process_beep_cmd },
	{ FCHAT_BEEP_REPLY_CMD, BLOCK_MSG, fchat_process_beep_reply_cmd },

	{ FCHAT_STATUS_CMD, BLOCK_MSG, fchat_process_status_cmd },
	{ FCHAT_ALIAS_CHANGE_CMD, BLOCK_MSG, fchat_process_alias_change_cmd },
	{ FCHAT_GET_BUDDY_INFO_CMD, 0, fchat_process_get_buddy_info_cmd },
	{ FCHAT_BUDDY_INFO_CMD, BLOCK_MSG, fchat_process_buddy_info_cmd },

	{ FCHAT_GET_MSG_BOARD_CMD, 0, fchat_process_get_msg_board_cmd },
	{ FCHAT_MSG_BOARD_CMD, BLOCK_MSG, fchat_process_msg_board_cmd },

	{ FCHAT_BUDDY_LIST_CMD, BLOCK_MSG, fchat_process_buddy_list_cmd },
	{ FCHAT_GET_BUDDY_LIST_CMD, 0, fchat_process_get_buddy_list_cmd },
	{ 0, 0, NULL }
};

static gchar *fchat_get_block_content(const gchar block_key, const gchar *packet) {
	gchar *text_block_begin = NULL;
	/* Получить содержимое блока. Если блока нет вернёт NULL
	 * В тектовом блоке могут быть любые символы, в том числе и символы команд.
	 * Поэтому мы долюны преврять такие ситуации.
	 * Для этого сначала определяем позицию текстового блока и проверям не
	 * находится ли начало блока в текстовом блоке.
	*/
	gchar needle[3];
	needle[0] = (gchar)FCHAT_SEPARATOR_BLOCK;
	needle[1] = (gchar)FCHAT_MSG_BLOCK;
	needle[2] = '\0';
	text_block_begin = strstr(packet, needle);

	if (block_key == (gchar)FCHAT_MSG_BLOCK) {
		if (!text_block_begin) {
			return NULL;
		} else {
			/* Смещаем указатель на два символа вперёд */
			text_block_begin += 2;
			return g_strdup(text_block_begin);
		}
	} else {
		needle[1] = block_key;
		gchar *begin_pos = strstr(packet, needle);
		/* Если блок не найден или находится в тексте */
		if (!begin_pos || (text_block_begin && (begin_pos >= text_block_begin))) {
			return NULL;
		} else {
			begin_pos += 2;
			gchar *end_pos = strchr(begin_pos, (gchar)FCHAT_SEPARATOR_BLOCK);
			if (!end_pos) {
				return g_strdup(begin_pos);
			} else {
				return g_strndup(begin_pos, end_pos - begin_pos);
			}
		}
	}
}

void fchat_process_packet(FChatConnection *fchat_conn, const gchar *host, const gchar *packet) {
	/* Команда обязательна. Если её нет - пакет битый или он для другой проги */
	gchar *command = fchat_get_block_content((gchar)FCHAT_COMMAND_BLOCK, packet);
	if (!command || (*command == '\0')) {
		purple_debug_warning("fchat", "Bad packet from host %s. There is not command in packet\n", host);
		return;
	}

	/* Вызываем обработчик этой команды */
	for (gint i = 0; fchat_msgs[i].command; i++) {
		if (fchat_msgs[i].command == command[0]) {
			/* Extract blocks */
			FChatPacketBlocks *packet_blocks = g_new0(FChatPacketBlocks, 1);
			packet_blocks->command = command;
			FChatPacketBlocksVector packet_blocks_v = (FChatPacketBlocksVector)packet_blocks;
			int base2 = 1;
			for (int block_num = 1; block_num < FCHAT_BLOCKS_COUNT; block_num++) {
				if (fchat_msgs[i].blocks_to_parse & base2) {
					packet_blocks_v[block_num] = fchat_get_block_content(fchat_blocks_order[block_num], packet);
				}
				base2 = base2 * 2;
			}
			fchat_debug_print_packet_blocks(fchat_conn, packet_blocks);
			FChatBuddy *buddy = fchat_find_buddy(fchat_conn, host, NULL, TRUE);
			buddy->last_packet_time = time(NULL);
			fchat_msgs[i].cb(fchat_conn, buddy, packet_blocks);
			fchat_delete_packet_blocks(packet_blocks);
			return;
		}
	}
	purple_debug_warning("fchat", "Unknown or unsupported command '%s' (%d)\n", command, command[0]);
}
