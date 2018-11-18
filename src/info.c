#include "fchat.h"

static gchar* extract_param(const gchar *from_codeset, const gchar *str_info, const gchar *key) {
	gchar needle[32];
	g_sprintf(needle, "%s%c", key, FCHAT_INFO_KEY_SEPARATOR);
	gchar *begin_pos = strstr(str_info, needle);
	if (!begin_pos) {
		return NULL;
	}

	begin_pos += strlen(key) + 1;
	gchar *end_pos = NULL;
	end_pos = strchr(begin_pos, FCHAT_INFO_KEY_SEPARATOR);
	gsize len = end_pos ? end_pos - begin_pos : -1;

	if (from_codeset) {
		return g_convert(begin_pos, len, "UTF-8", from_codeset, NULL, NULL, NULL);
	} else {
		if (len == -1) {
			return g_strdup(begin_pos);
		} else {
			return g_strndup(begin_pos, len);
		}
	}
}

FChatBuddyInfo *fchat_parse_buddy_info(const gchar *str_info, const gchar *from_codeset) {
// FChatVersion[2]4.6.1[2]FullName[2]Sergey Ponomarev[2]Male[2]1[2]Day[2]23[2]Month[2]12[2]Year[2]2000
// Личная иформация передаётся строкой где каждый параметр отделён символом #2
// В личной информации передаётся версия ФЧАТ (не путать с версией протокола!).
// Её мы деликатно игнорируем ибо это не персональная ифнормация

	FChatBuddyInfo *info = g_new0(FChatBuddyInfo, 1);
	info->full_name = extract_param(from_codeset, str_info, "FullName");

	gchar *male = extract_param(NULL, str_info, "Male");
	if (male) {
		info->male = (gboolean)(atoi(male)) ? FCHAT_BUDDY_MALE: FCHAT_BUDDY_FEMALE;
		g_free(male);
	} else {
		info->male = FCHAT_BUDDY_MALE_NOT_SPECIFIED;
	}

	gchar *num;
	num = extract_param(NULL, str_info, "Day");
	if (num) {
		info->birthday_day = atoi(num);
		g_free(num);
	} else {
		info->birthday_day = 0;
	}

	num = extract_param(NULL, str_info, "Month");
	if (num) {
		info->birthday_month = atoi(num);
		g_free(num);
	} else {
		info->birthday_month = 0;
	}

	num = extract_param(NULL, str_info, "Year");
	if (num) {
		info->birthday_year = atoi(num);
		g_free(num);
	} else {
		info->birthday_year = 0;
	}

	info->address = extract_param(from_codeset, str_info, "Address");
	info->phone = extract_param(from_codeset, str_info, "Phone");
	info->email = extract_param(from_codeset, str_info, "Email");
	info->interest= extract_param(from_codeset, str_info, "Interest");
	info->additional = extract_param(from_codeset, str_info, "Additional");
	return info;
}

static void add_str_param_to_info(GString *info, const gchar *key, const gchar *param, const gchar *from_codeset) {
	if (param) {
		g_string_append(info, key);
		g_string_append_c(info, FCHAT_INFO_KEY_SEPARATOR);
		gchar *converted_param = g_convert(param, -1, from_codeset, "UTF-8", NULL, NULL, NULL);
		g_string_append(info, converted_param);
		g_free(converted_param);
		g_string_append_c(info, FCHAT_INFO_KEY_SEPARATOR);
	}
}

static void add_int_param_to_info(GString *info, const gchar *key, gint param, gint null_val) {
	if (param != null_val) {
		g_string_append(info, key);
		g_string_append_c(info, FCHAT_INFO_KEY_SEPARATOR);
		gchar *str_param = g_strdup_printf("%d", param);
		g_string_append(info, str_param);
		g_free(str_param);
		g_string_append_c(info, FCHAT_INFO_KEY_SEPARATOR);
	}
}

gchar *fchat_buddy_info_serialize(FChatBuddyInfo *info, const gchar *from_codeset) {
	GString *data = g_string_new(NULL);

	add_str_param_to_info(data, "FChatVersion", FCHAT_MY_PROGRAM_VERSION, from_codeset);
	if (info != NULL) {
		add_str_param_to_info(data, "FullName", info->full_name, from_codeset);
		add_str_param_to_info(data, "Address",  info->address, from_codeset);
		add_str_param_to_info(data, "Phone",    info->phone, from_codeset);
		add_int_param_to_info(data, "Male", info->male, FCHAT_BUDDY_MALE_NOT_SPECIFIED);
		add_int_param_to_info(data, "Day", info->birthday_day, 0);
		add_int_param_to_info(data, "Month", info->birthday_month, 0);
		add_int_param_to_info(data, "Year", info->birthday_year, 0);
		add_str_param_to_info(data, "Email", info->email, from_codeset);
		add_str_param_to_info(data, "Interest", info->interest, from_codeset);
		add_str_param_to_info(data, "Additional", info->additional, from_codeset);
	}
	gchar *serialized_info = data->str;
	g_string_free(data, FALSE);
	return serialized_info;
}



PurpleNotifyUserInfo *fchat_buddy_info_to_purple_info(FChatBuddyInfo *info) {
	PurpleNotifyUserInfo *purple_info = purple_notify_user_info_new();
	purple_notify_user_info_add_pair(purple_info, _("Full name"), info->full_name);
	gchar *male = NULL;
	switch (info->male) {
		case FCHAT_BUDDY_MALE:
			male = g_strdup(_("Male"));
			break;
		case FCHAT_BUDDY_FEMALE:
			male = g_strdup(_("Female"));
			break;
		case FCHAT_BUDDY_MALE_NOT_SPECIFIED:
			male = NULL;
			break;
	}
	if (male) {
		purple_notify_user_info_add_pair(purple_info, _("Male"), male);
		g_free(male);
	}

	gchar *birthday = g_strdup_printf("%d %d %d", info->birthday_day, info->birthday_month, info->birthday_year);
	purple_notify_user_info_add_pair(purple_info, _("Birthday"), birthday);
	g_free(birthday);

	purple_notify_user_info_add_pair(purple_info, _("Address"), info->address);
	purple_notify_user_info_add_pair(purple_info, _("Phone"), info->phone);
	purple_notify_user_info_add_pair(purple_info, _("Email"), info->email);
	purple_notify_user_info_add_pair(purple_info, _("Interest"), info->interest);
	purple_notify_user_info_add_pair(purple_info, _("Additional"), info->additional);

	return purple_info;
}

/*
 * UI callbacks
 */

#define FCHAT_MIN_BIRTHDAY_YEAR 1950
#define FCHAT_MAX_BIRTHDAY_YEAR 2005

static void
fchat_format_info(PurpleConnection *gc, PurpleRequestFields *fields)
{
	PurpleRequestField *field;
	char *p;
	FChatConnection *fchat_conn = (FChatConnection *)gc->proto_data;
	FChatBuddyInfo *info = fchat_conn->my_buddy->info;

	fchat_buddy_info_destroy(info);
	info = g_new0(FChatBuddyInfo, 1);

	field = purple_request_fields_get_field(fields, "full_name");
	info->full_name = g_strdup(purple_request_field_string_get_value(field));

	field  = purple_request_fields_get_field(fields, "male");
	info->male = purple_request_field_choice_get_value(field);
	info->male = info->male - 1;

	field  = purple_request_fields_get_field(fields, "birthday_day");
	info->birthday_day = purple_request_field_choice_get_value(field);

	field  = purple_request_fields_get_field(fields, "birthday_month");
	info->birthday_month = purple_request_field_choice_get_value(field);

	field  = purple_request_fields_get_field(fields, "birthday_year");
	info->birthday_year = purple_request_field_choice_get_value(field);
	if (info->birthday_year > 0) {
		info->birthday_year += FCHAT_MIN_BIRTHDAY_YEAR - 1;
	}

	field  = purple_request_fields_get_field(fields, "address");
	info->address = g_strdup(purple_request_field_string_get_value(field));

	field  = purple_request_fields_get_field(fields, "phone");
	info->phone = g_strdup(purple_request_field_string_get_value(field));

	field = purple_request_fields_get_field(fields, "email");
	info->email = g_strdup(purple_request_field_string_get_value(field));

	field = purple_request_fields_get_field(fields, "interest");
	info->interest = g_strdup(purple_request_field_string_get_value(field));

	field = purple_request_fields_get_field(fields, "additional");
	info->additional = g_strdup(purple_request_field_string_get_value(field));

	GKeyFile *info_fields = g_key_file_new();
	g_key_file_set_string(info_fields, "info", "full_name", info->full_name);
	g_key_file_set_integer(info_fields, "info", "male", info->male);
	g_key_file_set_integer(info_fields, "info", "birthday_day", info->birthday_day);
	g_key_file_set_integer(info_fields, "info", "birthday_month", info->birthday_month);
	g_key_file_set_integer(info_fields, "info", "birthday_year", info->birthday_year);
	g_key_file_set_string(info_fields, "info", "address", info->address);
	g_key_file_set_string(info_fields, "info", "phone", info->phone);
	g_key_file_set_string(info_fields, "info", "email", info->email);
	g_key_file_set_string(info_fields, "info", "interest", info->interest);
	g_key_file_set_string(info_fields, "info", "additional", info->additional);
	p = g_key_file_to_data(info_fields, NULL, NULL);
	g_key_file_free(info_fields);

	purple_account_set_user_info(gc->account, p);
	serv_set_info(gc, p);

	g_free(p);
}

void fchat_action_set_user_info(PurplePluginAction *action)
{
	PurpleConnection *gc = (PurpleConnection *)action->context;
	PurpleAccount *account = purple_connection_get_account(gc);
	purple_debug_info("fchat", "showing 'Set User Info' dialog for %s\n",
			account->username);

	PurpleRequestFields *fields;
	PurpleRequestFieldGroup *group;
	PurpleRequestField *field;

	FChatConnection *fchat_conn = (FChatConnection *)gc->proto_data;
	FChatBuddyInfo *info = fchat_conn->my_buddy->info;

	fields = purple_request_fields_new();
	group = purple_request_field_group_new(NULL);
	purple_request_fields_add_group(fields, group);

	field = purple_request_field_string_new("full_name", _("Full name"), info->full_name, FALSE);
	purple_request_field_group_add_field(group, field);

	field = purple_request_field_choice_new("male", _("Sex"), info->male);
	purple_request_field_choice_add(field, _("Not specified"));
	purple_request_field_choice_add(field, _("Female"));
	purple_request_field_choice_add(field, _("Male"));
	purple_request_field_group_add_field(group, field);

	field = purple_request_field_choice_new("birthday_day", _("Birthday day"), info->birthday_day);
	purple_request_field_choice_add(field, "");
	gchar *day_num;
	gint i;
	for (i = 1; i <= 31; i++) {
		day_num = g_strdup_printf ("%d", i);
		purple_request_field_choice_add(field, day_num);
		g_free(day_num);
	}
	purple_request_field_group_add_field(group, field);

	field = purple_request_field_choice_new("birthday_month", _("Birthday month"), info->birthday_month);
	purple_request_field_choice_add(field, "");
	for (i = 1; i <= 12; i++) {
		day_num = g_strdup_printf ("%d", i);
		purple_request_field_choice_add(field, day_num);
		g_free(day_num);
	}
	purple_request_field_group_add_field(group, field);

	field = purple_request_field_choice_new("birthday_year", _("Birthday year"), info->birthday_year);
	purple_request_field_choice_add(field, "");

	for (i = FCHAT_MIN_BIRTHDAY_YEAR; i <= FCHAT_MAX_BIRTHDAY_YEAR; i++) {
		day_num = g_strdup_printf ("%d", i);
		purple_request_field_choice_add(field, day_num);
		g_free(day_num);
	}
	purple_request_field_group_add_field(group, field);


	field = purple_request_field_string_new("address", _("Address"), info->address, FALSE);
	purple_request_field_group_add_field(group, field);

	field = purple_request_field_string_new("phone", _("Phone"), info->phone, FALSE);
	purple_request_field_group_add_field(group, field);

	field = purple_request_field_string_new("email", _("E-mail"), info->email, FALSE);
	purple_request_field_group_add_field(group, field);

	field = purple_request_field_string_new("interest", _("Interest"), info->interest, TRUE);
	purple_request_field_group_add_field(group, field);

	field = purple_request_field_string_new("additional", _("Additional"), info->additional, TRUE);
	purple_request_field_group_add_field(group, field);

	purple_request_fields(gc, _("Edit info"),
		_("Edit your personal information"),
		_("All items below are optional. Enter only the "
		"information with which you feel comfortable."),
		fields,
		_("Save"), G_CALLBACK(fchat_format_info),
		_("Cancel"), NULL,
		account, NULL, NULL,
		gc);
}


FChatBuddyInfo *fchat_load_my_buddy_info(PurpleAccount *account) {
	FChatBuddyInfo *info = g_new0(FChatBuddyInfo, 1);
	const gchar *serialized_info = purple_account_get_user_info(account);
	if (!serialized_info) {
		return info;
	}

	GKeyFile *info_fields =  g_key_file_new();
	GError *error;
	if (g_key_file_load_from_data(info_fields, serialized_info, -1, G_KEY_FILE_NONE, &error)) {
		info->full_name = g_key_file_get_string(info_fields, "info", "full_name", NULL);
		info->male = g_key_file_get_integer(info_fields, "info", "male", NULL);
		info->birthday_day = g_key_file_get_integer(info_fields, "info", "birthday_day", NULL);
		info->birthday_month = g_key_file_get_integer(info_fields, "info", "birthday_month", NULL);
		info->birthday_year = g_key_file_get_integer(info_fields, "info", "birthday_year", NULL);
		info->address = g_key_file_get_string(info_fields, "info", "address", NULL);
		info->phone = g_key_file_get_string(info_fields, "info", "phone", NULL);
		info->email = g_key_file_get_string(info_fields, "info", "email", NULL);
		info->interest = g_key_file_get_string(info_fields, "info", "interest", NULL);
		info->additional = g_key_file_get_string(info_fields, "info", "additional", NULL);
	} else {
		purple_debug_error("fchat", "Can't load user info: %s\n", error->message);
	}
	g_key_file_free(info_fields);
	return info;
}

void fchat_buddy_info_destroy(FChatBuddyInfo *info) {
	if (info->full_name)
		g_free(info->full_name);
	if (info->address)
		g_free(info->address);
	if (info->phone)
		g_free(info->phone);
	if (info->email)
		g_free(info->email);
	if (info->interest)
		g_free(info->interest);
	if (info->additional)
		g_free(info->additional);
	g_free(info);
}


void fchat_prpl_get_info(PurpleConnection *gc, const gchar *who) {
	FChatConnection *fchat_conn = gc->proto_data;
	FChatBuddy *buddy = fchat_find_buddy(fchat_conn, who, NULL, TRUE);
	fchat_send_get_buddy_info_cmd(fchat_conn, buddy);
}
