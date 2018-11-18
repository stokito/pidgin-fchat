#ifndef _PURPLE_FCHAT_H_
#define _PURPLE_FCHAT_H_

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gprintf.h>

#define GETTEXT_PACKAGE "fchat"
#define LOCALEDIR "./locale"
#include <glib/gi18n-lib.h>

#include <gio/gio.h>
#include <purple.h>

#define FCHATPRPL_ID "prpl-fchat"
#define FCHAT_STATUS_ONLINE   "online"
#define FCHAT_STATUS_AWAY     "away"
#define FCHAT_STATUS_OFFLINE  "offline"

/*
 * Описание протокола FChat версии 4.6.1 (278)
 * Все данные перадются UDP пакетами (datagrams)
 * Каждый пакет разбит на блоки разделёные символом #1 и идентификатором блока.
 * Один блок с кодом #8 содержит команду, например сообщение, смена ника, запрос информации, проверка связи (пинг).
 * Остальные блоки используются в зависимости от команды.
 * Блок потдверждение сообщения хитрожопый. Он пустой. Но если он есть тогда требуется подтверждение сообщения.
 * Ограничения:
 * * Чат комната возможна только одна. В стандартном ФЧат приватные комнаты тоже непонтяно как работают.
 * * Шифрование не поддерживается. Килевич закрыл алгоритм да и не нужно оно.
 * * Удалить своё сообщение не получится. Подтвердить доставку тоже. Такого механизма нет в purple.
 * * Иконки статусов отсутсвуют. Их нужно реализовывать через xStatuses а они появятся в пиджене не скоро (если вообще появятся).
 * * Доска объявлений как таковая не реализована но можно читать объявление одного собеседника. Такого механизма нет в purple.
 */

#define FCHAT_MY_PROGRAM_VERSION "4.6.1"
// для 4.6.1, с каждой новой версией увеличивается на 1
#define FCHAT_MY_PROTOCOL_VERSION 278
// Порт получателя по умолчаню
#define FCHAT_DEFAULT_PORT 9999

#define FCHAT_CHAT_ROOM_NAME "Main"
#define FCHAT_CHAT_ROOM_ID   g_str_hash(FCHAT_CHAT_ROOM_NAME)

// Блок команды обязателен
#define FCHAT_SEPARATOR_BLOCK        1   /**< разделитель блоков сообщений                 */
#define FCHAT_PROTOCOL_VERSION_BLOCK 1   /**< Версия протокола                             */
#define FCHAT_ALIAS_BLOCK            2   /**< Имя контакта                                 */
#define FCHAT_COMMAND_BLOCK          8   /**< Код команды                                  */
#define FCHAT_MSG_ID_BLOCK           9   /**< Hомер сообщения                              */
#define FCHAT_MSG_CONFIRMATION_BLOCK 10  /**< Требование подтверждения о доставке (пустой) */
#define FCHAT_MSG_BLOCK              255 /**< Тект сообщения                               */

//#define EOL  \r\n               /**< Конец строки (Используется в автоответчиках) */
#define FCHAT_INFO_KEY_SEPARATOR   2
#define FCHAT_BUDDY_LIST_SEPARATOR "\3"

// коды сообщений
#define FCHAT_CONNECT_CMD         'C' /**< Запрос на соединение                    */
#define FCHAT_CONNECT_CONFIRM_CMD 'F' /**< Разрешение соединения                   */
#define FCHAT_PING_CMD            'X' /**< Проверка связи                          */
#define FCHAT_PONG_CMD            'Y' /**< Cвязь ОК                                */
#define FCHAT_DISCONNECT_CMD      'D' /**< Оповещение о отключении                 */
#define FCHAT_ALIAS_CHANGE_CMD    'e' /**< Изменение псевдонима                    */
#define FCHAT_GET_BUDDY_INFO_CMD  'u' /**< Получить информацию о собеседнике       */
#define FCHAT_BUDDY_INFO_CMD      'U' /**< Информация о собеседнике                */
#define FCHAT_GET_BUDDY_LIST_CMD  'L' /**< Запрос списка собеседников              */
#define FCHAT_BUDDY_LIST_CMD      'G' /**< Предоставление списка собеседников      */
#define FCHAT_MSG_CMD             'M' /**< Текст общий                             */
#define FCHAT_PRIVATE_MSG_CMD     'P' /**< Текст личный                            */
#define FCHAT_CONFIRM_MSG_CMD     'O' /**< Подтверждение доставки сообщения        */
#define FCHAT_DELETE_MSG_CMD      'J' /**< Удалить своё сообщение                  */
#define FCHAT_BEEP_CMD            'B' /**< Сигнал                                  */
#define FCHAT_BEEP_REPLY_CMD      'N' /**< Ответ на сигнал                         */
#define FCHAT_STATUS_CMD          'a' /**< Изменилось состояние автоответчика      */
#define FCHAT_GET_MSG_BOARD_CMD   '2' /**< Получить сообщение для доски объявлений */
#define FCHAT_MSG_BOARD_CMD       '3' /**< Сообщение для доски объявлений          */


/* Ключи шифрования мы не учитываем */
typedef struct {
	gchar *command;          /**< Код команды                                  */
	gchar *protocol_version; /**< Версия протокола                             */
	gchar *alias;            /**< Имя контакта                                 */
	gchar *msg_id;           /**< Hомер сообщения                              */
	gchar *msg_confirmation; /**< Требование подтверждения о доставке (пустой) */
	gchar *msg;              /**< Тект сообщения. Обязательно последний!       */
} FChatPacketBlocks;

#define FCHAT_BLOCKS_COUNT sizeof(FChatPacketBlocks) / sizeof(gchar *)

typedef gchar **FChatPacketBlocksVector;
static gchar fchat_blocks_order[FCHAT_BLOCKS_COUNT] = {
	FCHAT_COMMAND_BLOCK,
	FCHAT_PROTOCOL_VERSION_BLOCK,
	FCHAT_ALIAS_BLOCK,
	FCHAT_MSG_ID_BLOCK,
	FCHAT_MSG_CONFIRMATION_BLOCK,
	FCHAT_MSG_BLOCK
};

typedef enum {
	FCHAT_BEEP_ACEPTED,
	FCHAT_BEEP_DENYED_FROM_ALL,
	FCHAT_BEEP_DENYED_FROM_YOU
} FChatBeepReply;

typedef enum {
	FCHAT_MSG_TYPE_PUBLIC,
	FCHAT_MSG_TYPE_PRIVATE
} FChatMsgType;

typedef enum {
	FCHAT_BUDDIES_LIST_PRIVACY_REQUEST, // default
	FCHAT_BUDDIES_LIST_PRIVACY_ALLOW,
	FCHAT_BUDDIES_LIST_PRIVACY_DENY
} FChatBuddiesListPrivacy;

typedef enum {
	FCHAT_BUDDY_MALE_NOT_SPECIFIED = -1,
	FCHAT_BUDDY_MALE,
	FCHAT_BUDDY_FEMALE
} FChatBuddyMale;

typedef struct {
	gchar *full_name;
	FChatBuddyMale male;
	gint8 birthday_day;
	gint8 birthday_month;
	gint birthday_year;
	gchar *address;
	gchar *phone;
	gchar *email;
	gchar *interest;
	gchar *additional;
} FChatBuddyInfo;

typedef struct {
	gchar *host;
	GInetAddress *addr;
	gchar *alias;
	FChatBuddyInfo *info;
	gint protocol_version;
	time_t last_packet_time;
	gchar *msg_board;
} FChatBuddy;

typedef struct {
	PurpleConnection *gc;
	FChatBuddy *my_buddy;
	GSocket *socket;
	guint next_id;           /**< Next msg id required for confirmation */
	guint timer;
	gint keepalive_timeout;  /**< A purple timeout tag for the keepalive in minutes */
	gint8 keepalive_periods;
	GHashTable *buddies;
	GIOChannel *channel;
} FChatConnection;


void fchat_connection_delete(FChatConnection *fchat_conn);

void fchat_prpl_close(PurpleConnection *gc);
void fchat_prpl_login(PurpleAccount *account);
gboolean fchat_keepalive(gpointer data);

void fchat_prpl_add_buddy(PurpleConnection *gc, PurpleBuddy *buddy, PurpleGroup *group);
void fchat_prpl_remove_buddy(PurpleConnection *gc, PurpleBuddy *buddy, PurpleGroup *group);

FChatBuddy *fchat_buddy_new(const gchar *host, const gchar *alias);
FChatBuddy *fchat_find_buddy(FChatConnection *fchat_conn, const gchar *host, const gchar *alias, gboolean create);
void fchat_buddy_delete(gpointer p);
FChatBuddy **fchat_get_buddies_list_all(FChatConnection *fchat_conn);

typedef void FChatSelectBuddiesListCb(FChatConnection *fchat_conn, GHashTable *buddies, void *user_data);
void fchat_select_buddies_list(FChatConnection *fchat_conn, const gchar *msg_text, GHashTable *buddies, FChatSelectBuddiesListCb *ok_cb, FChatSelectBuddiesListCb *cancel_cb, void *user_data);

PurpleNotifyUserInfo *fchat_buddy_info_to_purple_info(FChatBuddyInfo *info);
FChatBuddyInfo *fchat_parse_buddy_info(const gchar *str_info, const gchar *from_codeset);
gchar *fchat_buddy_info_serialize(FChatBuddyInfo *, const gchar *from_codeset);
FChatBuddyInfo *fchat_load_my_buddy_info(PurpleAccount *account);
void fchat_load_buddy_list(FChatConnection *fchat_conn);
void fchat_buddy_info_destroy(FChatBuddyInfo *info);

const gchar *fchat_get_connection_codeset(FChatConnection *fchat_conn);
gchar *fchat_encode(FChatConnection *fchat_conn, const gchar *str, gssize len);
gchar *fchat_decode(FChatConnection *fchat_conn, const gchar *str, gssize len);

FChatPacketBlocks *fchat_parse_packet_blocks(const gchar *packet);
void fchat_process_packet(FChatConnection *fchat_conn, const gchar *host, const gchar *packet);

GString *fchat_make_packet(FChatPacketBlocks *packet_blocks);
void fchat_delete_packet_blocks(FChatPacketBlocks *packet_blocks);
void fchat_debug_print_packet_blocks(FChatConnection *fchat_conn, FChatPacketBlocks *packet_blocks);

void fchat_send_connect_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy);
void fchat_send_connect_confirm_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, gboolean can_connect);
void fchat_send_disconnect_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy);
void fchat_send_ping_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy);
void fchat_send_pong_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy);
void fchat_send_confirm_msg_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, gchar *msg_id);
void fchat_send_change_alias_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, const gchar *new_nickname);
void fchat_send_msg_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, const gchar *msg_text,  FChatMsgType msg_type, gboolean msg_confirmation);
void fchat_send_beep_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy);
void fchat_send_beep_reply_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy,  FChatBeepReply reply);
void fchat_send_get_buddy_info_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy);
void fchat_send_my_buddy_info_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy);
void fchat_send_status_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy, PurpleStatus *status);
void fchat_send_get_msg_board_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy);
void fchat_send_msg_board_cmd(FChatConnection *fchat_conn, FChatBuddy *buddy);
void fchat_send_get_buddies(FChatConnection *fchat_conn, FChatBuddy *buddy);
void fchat_send_buddies(FChatConnection *fchat_conn, FChatBuddy *buddy, GHashTable *buddies);

typedef struct {
	FChatConnection *fchat_conn;
	FChatBuddy *buddy;
} FChatRequestAuthorizationCbData;
void fchat_request_authorization_allow_cb(void *user_data);
void fchat_request_authorization_deny_cb(void *user_data);

gchar *fchat_prpl_status_text(PurpleBuddy *buddy);
GList *fchat_prpl_status_types(PurpleAccount *account);
void fchat_prpl_set_status(PurpleAccount *account, PurpleStatus *status);

void fchat_prpl_get_info(PurpleConnection *gc, const gchar *who);

GList *fchat_prpl_chat_info(PurpleConnection *gc);
GHashTable *fchat_prpl_chat_info_defaults(PurpleConnection *gc, const gchar *room);
gchar *fchat_prpl_chat_get_name(GHashTable *components);
void fchat_prpl_chat_join(PurpleConnection *gc, GHashTable *components);
PurpleRoomlist *fchat_prpl_chat_get_roomlist(PurpleConnection *gc);
gint fchat_prpl_chat_send(PurpleConnection *gc, gint id, const gchar *message, PurpleMessageFlags flags);

GList *fchat_prpl_actions(PurplePlugin *plugin, gpointer context);
void fchat_action_set_user_info(PurplePluginAction *action);

GList *fchat_prpl_blist_node_menu(PurpleBlistNode *node);

gboolean fchat_prpl_send_attention(PurpleConnection *gc, const gchar *username, guint type);
GList *fchat_prpl_get_attention_types(PurpleAccount *account);

#endif /* _PURPLE_FCHAT_H_ */
