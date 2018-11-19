#include "fchat.h"

const gchar *fchat_get_connection_codeset(FChatConnection *fchat_conn) {
	const gchar *codeset = purple_account_get_string(fchat_conn->gc->account, "codeset", NULL);
	if (!codeset) {
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
	 * По идее мы берём и забираем из канала пришедшие байты:
	 * g_io_channel_read_chars(iochannel, buffer, sizeof (buffer), &bytes_received, &error);
	 * Но беда в том что нам нужен айпишник приславшего пакет.
	 * Поэтому выбирать мы будем не через абстрактный канал, а прямо из сокета.
	 */
	FChatConnection *fchat_conn = (FChatConnection *)data;
	gchar buffer[1024];
	GError *error = NULL;
	GSocketAddress *address = NULL;
	gssize bytes_received = g_socket_receive_from(fchat_conn->socket, &address, buffer, sizeof(buffer), NULL, &error);
	if (bytes_received <= 0) {
		purple_debug_error("fchat", "fchat_receive_packet: bytes_received=%ld\n", bytes_received);
		g_clear_object(&address);
		return TRUE;
	}
	GInetSocketAddress *sender = G_INET_SOCKET_ADDRESS(address);
	gchar *packet = g_strndup(buffer, (gsize) bytes_received);
	gchar *host = g_inet_address_to_string(g_inet_socket_address_get_address(sender));
	fchat_process_packet(fchat_conn, host, packet);
	g_free(packet);
	g_free(host);
	g_clear_object(&sender);
	return TRUE;
}

static gboolean fchat_receive_packet_error(GIOChannel *iochannel, GIOCondition c, gpointer data) {
	purple_debug_error("fchat", "fchat_receive_packet_error: GIOCondition c=%d\n", c);
	return FALSE;
}

static void func_send_connects(gpointer key, gpointer value, gpointer user_data) {
	fchat_send_connect_cmd((FChatConnection *)user_data, (FChatBuddy *)value);
}

void fchat_prpl_login(PurpleAccount *account) {
	PurpleConnection *gc = purple_account_get_connection(account);
	FChatConnection *fchat_conn = gc->proto_data = g_new0(FChatConnection, 1);
	fchat_conn->gc = gc;

	guint16 port = (guint16) purple_account_get_int(fchat_conn->gc->account, "port", FCHAT_DEFAULT_PORT);
	gchar *host = account->username;
	fchat_conn->my_buddy = fchat_buddy_new(host, account->alias);
	if (!fchat_conn->my_buddy->addr) {
		purple_connection_error_reason(gc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, _("Incorrect address"));
		return;
	}
	fchat_conn->my_buddy->protocol_version = FCHAT_MY_PROTOCOL_VERSION;
	fchat_conn->my_buddy->info = fchat_load_my_buddy_info(account);

	// Создаём сокет на заданом в настроках айпишнике и порте
	GError *error = NULL;
	fchat_conn->socket = g_socket_new(G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_DATAGRAM, G_SOCKET_PROTOCOL_UDP, &error);
	if (fchat_conn->socket) {
		purple_connection_set_state(gc, PURPLE_CONNECTING);
	} else {
		purple_connection_error_reason(gc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, _("Couldn't create UDP socket"));
		purple_debug_error("fchat", "Couldn't create UDP socket (%d): %s", port, error->message);
		g_clear_error(&error);
		return;
	}

//	GSocketAddress *socket_address = g_inet_socket_address_new(g_inet_address_new_any(G_SOCKET_FAMILY_IPV4), port);
	GSocketAddress *socket_address = G_SOCKET_ADDRESS(g_inet_socket_address_new(fchat_conn->my_buddy->addr, port));
	if (!socket_address) {
		purple_connection_error_reason(gc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, _("Socket addr is incorrect"));
	}
	if (g_socket_bind(fchat_conn->socket, socket_address, TRUE, &error) == FALSE) {
		purple_connection_error_reason(gc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, _("Error bind"));
		purple_debug_error("fchat", "Couldn't bind UDP socket (%d): %s", port, error->message);
		g_clear_error(&error);
		return;
	}

	int fd = g_socket_get_fd(fchat_conn->socket);
	fchat_conn->channel = g_io_channel_unix_new(fd);
	g_io_channel_set_close_on_unref(fchat_conn->channel, TRUE);

	g_io_add_watch(fchat_conn->channel, G_IO_IN | G_IO_PRI, (GIOFunc) fchat_receive_packet, fchat_conn);
	// G_IO_NVAL мы не перехватываем поскольку когда удаляетя сокет дискриптор закрывается а потом он ещё закрываетя через канал и мы отхватываем ошибку
	g_io_add_watch(fchat_conn->channel, G_IO_ERR | G_IO_HUP, (GIOFunc) fchat_receive_packet_error, fchat_conn);
	fchat_conn->next_id = 0;

	fchat_conn->buddies = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, fchat_buddy_delete);
	fchat_load_buddy_list(fchat_conn);

	fchat_conn->keepalive_periods = (gint8) purple_account_get_int(gc->account, "keepalive_periods", 3);
	fchat_conn->keepalive_timeout = purple_account_get_int(gc->account, "keepalive_timeout", 1);
	fchat_conn->timer = purple_timeout_add_seconds((guint) (fchat_conn->keepalive_timeout * 60), fchat_keepalive, fchat_conn);

	// tell purple about everyone on our buddy list who's connected
	if (purple_account_get_bool(account, "broadcast", TRUE)) {
		FChatBuddy *broadcast_buddy = fchat_buddy_new("255.255.255.255", NULL);
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
	// notify other buddies
	g_hash_table_foreach(fchat_conn->buddies, func_send_disconnects, fchat_conn);
	fchat_connection_delete(fchat_conn);
}

gboolean fchat_keepalive(gpointer data) {
	FChatConnection *fchat_conn = data;
	purple_debug_info("fchat", "fchat_prpl_keepalive %d %d\n", fchat_conn->keepalive_timeout, fchat_conn->keepalive_periods);
	fchat_conn->timer = purple_timeout_add_seconds((guint) (fchat_conn->keepalive_timeout * 60), fchat_keepalive, fchat_conn);

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
	g_clear_object(&fchat_conn->socket);
	g_io_channel_unref(fchat_conn->channel);
	if (fchat_conn->buddies)
		g_hash_table_destroy(fchat_conn->buddies);
	fchat_buddy_delete(fchat_conn->my_buddy);
	g_free(fchat_conn);
}

void fchat_delete_packet_blocks(FChatPacketBlocks *packet_blocks) {
	FChatPacketBlocksVector packet_blocks_v = (FChatPacketBlocksVector)packet_blocks;
	for (int block_num = 0; block_num < FCHAT_BLOCKS_COUNT; block_num++) {
		gchar *block = packet_blocks_v[block_num];
		g_free(block);
	}
	g_free(packet_blocks);
}
