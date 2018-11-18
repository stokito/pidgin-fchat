#include "fchat.h"

static PurplePlugin *_fchat_protocol = NULL;

void fchat_debug_print_packet_blocks(FChatConnection *fchat_conn, FChatPacketBlocks *packet_blocks) {
	gchar *debug_msg = g_strdup_printf("packet blocks:\n"
			"protocol_version=%s\n"
			"nick_name=%s\n"
			"command=%s\n"
			"msg_id=%s\n"
			"msg_confirmation=%s\n"
			"msg=%s\n",
			packet_blocks->protocol_version, packet_blocks->alias, packet_blocks->command,
			packet_blocks->msg_id, packet_blocks->msg_confirmation, packet_blocks->msg);
	gchar *debug_msg_decoded = fchat_decode(fchat_conn, debug_msg, -1);
	g_free(debug_msg);
	purple_debug_info("fchat", "%s", debug_msg_decoded);
	g_free(debug_msg_decoded);
}


/*
 * prpl functions
 */
static const gchar *fchat_prpl_list_icon(PurpleAccount *account, PurpleBuddy *buddy) {
	return "fchat";
}

static gint fchat_prpl_send_im(PurpleConnection *gc, const gchar *who, const gchar *message, PurpleMessageFlags flags) {
	FChatConnection *fchat_conn = (FChatConnection *)gc->proto_data;
	FChatBuddy *buddy = fchat_find_buddy(fchat_conn, who, TRUE);
	gboolean msg_confirmation = purple_account_get_bool(gc->account, "confirm_private_msg", TRUE);
	fchat_send_msg_cmd(fchat_conn, buddy, message, FCHAT_MSG_TYPE_PRIVATE, msg_confirmation);
	return 1;
}


/*
 * prpl stuff. see prpl.h for more information.
 */

static PurplePluginProtocolInfo prpl_info = {
	OPT_PROTO_NO_PASSWORD | OPT_PROTO_REGISTER_NOSCREENNAME, /* options */
	NULL,                             /* user_splits, initialized in fchat_init() */
	NULL,                             /* protocol_options, initialized in fchat_init() */
	NO_BUDDY_ICONS,                   /* icon_spec */
	fchat_prpl_list_icon,             /* list_icon */
	NULL,                             /* list_emblem */
	fchat_prpl_status_text,           /* status_text */
	NULL,                             /* tooltip_text */
	fchat_prpl_status_types,          /* status_types */
	fchat_prpl_blist_node_menu,       /* blist_node_menu */
	fchat_prpl_chat_info,             /* chat_info */
	fchat_prpl_chat_info_defaults,    /* chat_info_defaults */
	fchat_prpl_login,                 /* login */
	fchat_prpl_close,                 /* close */
	fchat_prpl_send_im,               /* send_im */
	NULL,                             /* set_info */
	NULL,                             /* send_typing */
	fchat_prpl_get_info,              /* get_info */
	fchat_prpl_set_status,            /* set_status */
	NULL,                             /* set_idle */
	NULL,                             /* change_passwd */
	fchat_prpl_add_buddy,             /* add_buddy */
	NULL,                             /* add_buddies */
	fchat_prpl_remove_buddy,          /* remove_buddy */
	NULL,                             /* remove_buddies */
	NULL,                             /* add_permit */
	NULL,                             /* add_deny */
	NULL,                             /* rem_permit */
	NULL,                             /* rem_deny */
	NULL,                             /* set_permit_deny */
	fchat_prpl_chat_join,             /* join_chat */
	NULL,                             /* reject_chat */
	fchat_prpl_chat_get_name,         /* get_chat_name */
	NULL,                             /* chat_invite */
	NULL,                             /* chat_leave */
	NULL,                             /* chat_whisper */
	fchat_prpl_chat_send,             /* chat_send */
	NULL,                             /* keepalive */
	NULL,                             /* register_user */
	NULL,                             /* get_cb_info */
	NULL,                             /* get_cb_away */
	NULL,                             /* alias_buddy - user set alias for contact */
	NULL,                             /* group_buddy */
	NULL,                             /* rename_group */
	NULL,                             /* buddy_free */
	NULL,                             /* convo_closed */
	NULL,                             /* normalize */
	NULL,                             /* set_buddy_icon */
	NULL,                             /* remove_group */
	NULL,                             /* get_cb_real_name */
	NULL,                             /* set_chat_topic */
	NULL,                             /* find_blist_chat */
	fchat_prpl_chat_get_roomlist,     /* roomlist_get_list */
	NULL,                             /* roomlist_cancel */
	NULL,                             /* roomlist_expand_category */
	NULL,                             /* can_receive_file */
	NULL,                             /* send_file */
	NULL,                             /* new_xfer */
	NULL,                             /* offline_message */
	NULL,                             /* whiteboard_prpl_ops */
	NULL,                             /* send_raw */
	NULL,                             /* roomlist_room_serialize */
	NULL,                             /* unregister_user */
	fchat_prpl_send_attention,        /* send_attention */
	fchat_prpl_get_attention_types,   /* get_attention_types */
	sizeof(PurplePluginProtocolInfo), /* struct_size */
	NULL
};

static void fchat_init(PurplePlugin *plugin) {
	// see accountopt.h for information about user splits and protocol options
	PurpleAccountOption *option;

	// Кодировка из локалии
	const gchar *charset;
	g_get_charset (&charset);
	option = purple_account_option_string_new(_("Codeset"), "codeset", charset);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_int_new(_("Port"), "port", FCHAT_DEFAULT_PORT);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_bool_new(_("Use broadcast"), "broadcast", TRUE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_bool_new(_("Accept unknown protocol version"), "accept_unknown_protocol_version", TRUE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_int_new(_("Keepalive interval"), "keepalive_timeout", 1);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_int_new(_("Keepalive periods"), "keepalive_periods", 3);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_bool_new(_("Require authorization for new contacts"), "require_authorization", FALSE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_bool_new(_("Confirm private messages"), "confirm_private_msg", TRUE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_bool_new(_("Confirm public messages"), "confirm_public_msg", FALSE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_bool_new(_("Allow beep"), "accept_beep", TRUE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	GList *buddy_list_privacy = NULL;
	PurpleKeyValuePair *kvp;
	kvp = g_new0(PurpleKeyValuePair, 1);
	kvp->key = g_strdup(_("Deny"));
	kvp->value = g_strdup_printf("%d", FCHAT_BUDDIES_LIST_PRIVACY_DENY);
	buddy_list_privacy = g_list_append(buddy_list_privacy, kvp);
	kvp = g_new0(PurpleKeyValuePair, 1);
	kvp->key = g_strdup(_("Request"));
	kvp->value = g_strdup_printf("%d", FCHAT_BUDDIES_LIST_PRIVACY_REQUEST);
	buddy_list_privacy = g_list_append(buddy_list_privacy, kvp);
	kvp = g_new0(PurpleKeyValuePair, 1);
	kvp->key = g_strdup(_("Allow"));
	kvp->value = g_strdup_printf("%d", FCHAT_BUDDIES_LIST_PRIVACY_ALLOW);
	buddy_list_privacy = g_list_append(buddy_list_privacy, kvp);

	option = purple_account_option_list_new(_("Buddy list privacy"), "buddy_list_privacy", buddy_list_privacy);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	_fchat_protocol = plugin;
}

gboolean fchat_prpl_load(PurplePlugin *plugin) {
	return TRUE;
}

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,                                 /* magic */
	PURPLE_MAJOR_VERSION,                                /* major_version */
	PURPLE_MINOR_VERSION,                                /* minor_version */
	PURPLE_PLUGIN_PROTOCOL,                              /* type */
	NULL,                                                /* ui_requirement */
	0,                                                   /* flags */
	NULL,                                                /* dependencies */
	PURPLE_PRIORITY_DEFAULT,                             /* priority */
	FCHATPRPL_ID,                                        /* id */
	"FChat",                                             /* name */
	"1.0",                                               /* version */
	N_("FChat protocol plugin"),                         /* summary */
	N_("FChat protocol plugin"),                         /* description */
	N_("Sergey Ponomarev stokito.com"),                  /* author */
	"https://github.com/stokito/fchat-pidgin",           /* homepage */
	fchat_prpl_load,                                     /* load */
	NULL,                                                /* unload */
	NULL,                                                /* destroy */
	NULL,                                                /* ui_info */
	&prpl_info,                                          /* extra_info */
	NULL,                                                /* prefs_info */
	fchat_prpl_actions,                                  /* actions */
	NULL,                                                /* padding... */
	NULL,
	NULL,
	NULL,
};

PURPLE_INIT_PLUGIN(fchat, fchat_init, info);
