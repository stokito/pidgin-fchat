#include "fchat.h"

const gchar *fchat_get_connection_codeset(FChatConnection *fchat_conn) {
	const gchar *codeset = purple_account_get_string(fchat_conn->gc->account, "codeset", NULL);
	if (codeset == NULL) {
		g_get_charset(&codeset);
	}
	return codeset;
}

gchar *fchat_encode(FChatConnection *fchat_conn, const gchar *str, gssize len) {
	/* utf8 -> connection_codeset */
	GError *error = NULL;
	gchar *encoded_str = g_convert(str, len, fchat_get_connection_codeset(fchat_conn), "UTF-8", NULL, NULL, &error);
	if (error) {
		purple_debug_error("fchat", "Couldn't convert string: %s\n", error->message);
		g_error_free(error);
	}
	return encoded_str;
}

gchar *fchat_decode(FChatConnection *fchat_conn, const gchar *str, gssize len) {
	/* connection_codeset -> utf8 */
	GError *error = NULL;
	gchar *decoded_str = g_convert(str, len, "UTF-8", fchat_get_connection_codeset(fchat_conn), NULL, NULL, &error);
	if (error) {
		purple_debug_error("fchat", "Couldn't convert string: %s\n", error->message);
		g_error_free(error);
	}
	return decoded_str;
}

static gboolean fchat_receive_packet(GIOChannel *iochannel, GIOCondition c, gpointer data) {
	/**
	 * Это функция вызывается когда в канал сокета приходит пакет.
	 * По идее мы берём и забираем из канала пришедшие байты.
	 * Но беда в том что нам нужен айпишник приславшего пакет.
	 * Поэтому выбирать мы будем не через абстрактный канал, а прямо из сокета.
	 */
	FChatConnection *fchat_conn = (FChatConnection *)data;
	gchar buffer[1024];
	GInetAddr *sender = NULL;
	gint bytes_received = gnet_udp_socket_receive(fchat_conn->socket, buffer, sizeof(buffer), &sender);
	if (bytes_received == -1) {
		purple_debug_error("fchat", "fchat_receive_packet: bytes_received=%d\n", bytes_received);
		if (sender)
			gnet_inetaddr_delete(sender);
		return TRUE;
	}
	gchar *packet = g_strndup(buffer, bytes_received);
	gchar *host = gnet_inetaddr_get_canonical_name(sender);
	fchat_process_packet(fchat_conn, host, packet);
	g_free(packet);
	g_free(host);
	return TRUE;
}

static gboolean fchat_receive_packet_error(GIOChannel *iochannel, GIOCondition c, gpointer data) {
	purple_debug_warning("fchat", "fchat_receive_packet_error: GIOCondition c=%d\n", c);
}

static void func_send_connects(gpointer key, gpointer value, gpointer user_data) {
	fchat_send_connect_cmd((FChatConnection *)user_data, (FChatBuddy *)value);
}

void fchat_prpl_login(PurpleAccount *account) {
	PurpleConnection *gc = purple_account_get_connection(account);
	FChatConnection *fchat_conn = gc->proto_data = g_new0(FChatConnection, 1);
	fchat_conn->gc = gc;

	gint port = purple_account_get_int(fchat_conn->gc->account, "port", FCHAT_DEFAULT_PORT);
	gchar *host = account->username;
	fchat_conn->my_buddy = fchat_buddy_new(host, port);
	if (fchat_conn->my_buddy->addr == NULL) {
		purple_connection_error_reason(gc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, _("Incorrect address"));
		return;
	}
	fchat_conn->my_buddy->alias = g_strdup(account->alias); // nick_name;
	fchat_conn->my_buddy->protocol_version = FCHAT_MY_PROTOCOL_VERSION;
	fchat_conn->my_buddy->info = fchat_load_my_buddy_info(account);

	// Создаём сокет на заданом в настроках айпишнике и порте
	fchat_conn->socket = gnet_udp_socket_new_full(fchat_conn->my_buddy->addr, port);
	if (fchat_conn->socket != NULL) {
		purple_connection_set_state(gc, PURPLE_CONNECTING);
	} else {
		purple_connection_error_reason(gc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, _("Couldn't create UDP socket"));
		return;
	}
	GIOChannel* iochannel = gnet_udp_socket_get_io_channel(fchat_conn->socket);
	g_io_add_watch(iochannel, G_IO_IN | G_IO_PRI, fchat_receive_packet, fchat_conn);
	// G_IO_NVAL мы не перехватываем поскольку когда удаляетя сокет дискриптор закрывается а потом он ещё закрываетя через канал и мы отхватываем ошибку
	g_io_add_watch(iochannel, G_IO_ERR | G_IO_HUP, fchat_receive_packet_error, fchat_conn);
	fchat_conn->next_id = 0;

	fchat_conn->buddies = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, fchat_buddy_delete);
	fchat_load_buddy_list(fchat_conn);

	fchat_conn->keepalive_periods = purple_account_get_int(gc->account, "keepalive_periods", 3);
	fchat_conn->keepalive_timeout = purple_account_get_int(gc->account, "keepalive_timeout", 1);;
	fchat_conn->timer = purple_timeout_add_seconds(fchat_conn->keepalive_timeout * 60, fchat_keepalive, fchat_conn);

	// tell purple about everyone on our buddy list who's connected
	if (purple_account_get_bool(account, "broadcast", TRUE)) {
		FChatBuddy *broadcast_buddy = fchat_buddy_new("255.255.255.255", port);
		fchat_send_connect_cmd(fchat_conn, broadcast_buddy);
		fchat_buddy_delete(broadcast_buddy);
	} else {
		g_hash_table_foreach(fchat_conn->buddies, func_send_connects, fchat_conn);
	}

	purple_connection_set_state(gc, PURPLE_CONNECTED);
}

static void func_send_disconnects(gpointer key, gpointer value, gpointer user_data) {
	fchat_send_disconnect_cmd((FChatConnection *)user_data, (FChatBuddy *)value);
}

void fchat_prpl_close(PurpleConnection *gc) {
	FChatConnection *fchat_conn = gc->proto_data;
	g_return_if_fail(fchat_conn != NULL);
	// notify other buddy
	g_hash_table_foreach(fchat_conn->buddies, func_send_disconnects, fchat_conn);
	fchat_connection_delete(fchat_conn);
}

gboolean fchat_keepalive(gpointer data) {
	FChatConnection *fchat_conn = data;
	purple_debug_warning("fchat", "fchat_prpl_keepalive %d %d\n", fchat_conn->keepalive_timeout, fchat_conn->keepalive_periods);
	fchat_conn->timer = purple_timeout_add_seconds(fchat_conn->keepalive_timeout * 60, fchat_keepalive, fchat_conn);

	GHashTableIter iter;
	gchar *buddy_host;
	FChatBuddy *buddy;
	g_hash_table_iter_init (&iter, fchat_conn->buddies);
	while (g_hash_table_iter_next (&iter, (gpointer *)&buddy_host, (gpointer *)&buddy)) {
		if (time(NULL) - buddy->last_packet_time >= fchat_conn->keepalive_periods * fchat_conn->keepalive_timeout * 60) {
			purple_prpl_got_user_status(fchat_conn->gc->account, buddy->host, FCHAT_STATUS_OFFLINE, NULL);
		} else if (time(NULL) - buddy->last_packet_time >= fchat_conn->keepalive_timeout * 60) {
			fchat_send_ping_cmd(fchat_conn, buddy);
		}
	}
	return FALSE;
}

void fchat_connection_delete(FChatConnection *fchat_conn) {
	if (fchat_conn->timer)
		purple_timeout_remove(fchat_conn->timer);
	if (fchat_conn->socket)
		gnet_udp_socket_delete(fchat_conn->socket);
	if (fchat_conn->buddies)
		g_hash_table_destroy(fchat_conn->buddies);
	if (fchat_conn->my_buddy)
		fchat_buddy_delete(fchat_conn->my_buddy);
	g_free(fchat_conn);
}

void fchat_delete_packet_blocks(FChatPacketBlocks *packet_blocks) {
	FChatPacketBlocksVector packet_blocks_v = (FChatPacketBlocksVector)packet_blocks;
	gchar *block;
	int block_num;
	for (block_num = 0; block_num <= FCHAT_BLOCKS_COUNT; block_num++) {
		block = packet_blocks_v[block_num];
		if (block)
			g_free(block);
	}
	g_free(packet_blocks);
}
