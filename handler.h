/* 
 * File:   conn_model.h
 * Author: khoai
 *
 * Created on April 7, 2014, 5:54 PM
 */

#ifndef CONN_MODEL_H
#define	CONN_MODEL_H

#include "connmap.h"
#include "room.h"

#define CHAT_ROOM_ENABLE 0

void add_last_char(UT_string *response) {
    // <!> fix me, dirty code
    UT_string *temp;
    utstring_new(temp);
    utstring_clear(temp);
    utstring_printf(temp, "\n> ");
    utstring_concat(response, temp);
    utstring_free(temp);
}

int gen_user_list(UT_string* response) {
    UT_string *temp;
    connection *conn_user;
    utstring_new(temp);
    utstring_printf(response, "[+] user list: %d\n", connmap_count());

    for (conn_user = conmap_get(); conn_user != NULL; conn_user = conn_user->hh.next) {
        if (conn_user->state == AUTHEN) {
            utstring_clear(temp);
            utstring_printf(temp, "\t- %s\n", conn_user->user);
            utstring_concat(response, temp);
        }
    }
    utstring_free(temp);
}

int build_message(UT_string *res, char *message) {
    utstring_printf(res, "%s", message);
}

uint32_t get_next_param(uint8_t *in, uint32_t len, uint8_t **out) {
    uint32_t ret_len = 0;
    uint8_t *pos = strchr(in, ' ');
    if (pos > 0) {
        *out = ++pos;
        ret_len = len - ((uint32_t) (pos - in));
        return ret_len;
    }
    return 0;
}

// proc center

int proc_message(connection* conn, uint8_t *data, uint32_t len) {
    UT_string *response;
    uint8_t is_res = 1;

    //-- quit
    if (!strncmp(data, "quit", 4)) {
        close(conn->fd);
        return 1;
    }

    utstring_new(response);
    if (!strncmp(data, "ls", 2)) {
        gen_user_list(response);
    }
#if CHAT_ROOM_ENABLE
    else if (!strncmp(data, "join", 4)) {
        uint8_t *name = NULL;
        uint32_t name_len = get_next_param(data, len, &name);
        int ret_join;
        if (name_len > 0) {
            ret_join = room_join(name, name_len, conn);
            if (ret_join < 0) {
                utstring_printf(response, sz_join_room_err, name);
            } else if (ret_join == ADMIN) {
                utstring_printf(response, sz_join_room_admin, name);
            } else {
                utstring_printf(response, sz_join_room_user, name);
            }
        } else {
            build_message(response, "incorrect command");
        }
    } else if (!strncmp(data, "left", 4)) {
        room_left(conn);
    } else if (!strncmp(data, "passwd", 6)) {
        //
    } else if (conn->room_session) {
        broadcast_message_room(conn, data, len);
    } else {
        build_message(response, sz_plz_join_a_room);
    }
#else
    else {
        broadcast_message(conn, data, len);
        is_res = 0;
    }
#endif

    if (is_res) {
        add_last_char(response);
        conn_write(conn, utstring_body(response), utstring_len(response));
    }
    utstring_free(response);
    return 0;
}

#if CHAT_ROOM_ENABLE

int broadcast_message_room(connection *conn_from, uint8_t *data, uint32_t len) {
    room *it_room;
    UT_string *response;
    utstring_new(response);
    room *rm;

    rm = room_get_by_id(conn_from->room_session);
    if (!rm) {
        log_warn("room not found: %d", conn_from->room_session);
        return 1;
    }

    for (it_room = room_head(); it_room != NULL; it_room = it_room->hh.next) {
        if (it_room->state == AUTHEN) {
            // broadcast without me
            utstring_clear(response);
            if (conn_from != it_room)
                utstring_printf(response, "%s : %s", conn_from->user, data);
            add_last_char(response);
            conn_write(it_room, utstring_body(response), utstring_len(response));
        }
    }
    utstring_free(response);
    return 0;
}
#endif

int broadcast_message(connection *conn_from, uint8_t *data, uint32_t len) {
    connection *conn;
    UT_string *response;
    utstring_new(response);

    for (conn = conmap_get(); conn != NULL; conn = conn->hh.next) {
        if (conn->state == AUTHEN) {
            // broadcast without me
            utstring_clear(response);
            if (conn_from != conn) {
                utstring_printf(response, "%s: %s", conn_from->user, data);
                add_last_char(response);
//                conn_write(conn, utstring_body(response), utstring_len(response));
            } else {
                utstring_printf(response, "> ");
            }
            conn_write(conn, utstring_body(response), utstring_len(response));
        }
    }
    utstring_free(response);
    return 0;
}

int handler_read(connection* conn, uint8_t *data, uint32_t len) {
    UT_string *response;
    if (!conn) {
        log_warn("conn_read: conn == NULL");
        return 1;
    }
    len = stardard_message(data, len);
    if (!len) {
        return -1;
    }

    switch (conn->state) {
        case UNINIT:
            conn->user = malloc(len + 1);
            memset(conn->user, 0, len + 1);
            memcpy(conn->user, data, len + 1);
            
            stardard_user_name(conn->user, len);
            conn->state = AUTHEN;

            utstring_new(response);
            utstring_printf(response, sz_authen, conn->user);
            conn_write(conn, utstring_body(response), utstring_len(response));
            utstring_free(response);
            log_info("[-] [%s] joined public", conn->user);
            break;
        case AUTHEN:
            proc_message(conn, data, len);
            break;
        case DIE:
            break;
    }
    return 0;
}

int handler_new_conn(int fd) {
    connection* conn;
    if (conn = connmap_add(fd)) {
        // greeting
        conn_write(conn, sz_greet, sizeof (sz_greet));
    }
    return 0;
}

int handler_write(connection* conn) {
    return 0;
}

int handler_close(connection* conn) {
    if (!conn) {
        log_warn("connmap_get NULL");
        return 1;
    }
    log_info("[-] [%s] left", conn->user);
    conn_close(conn);
    connmap_remove(conn);
    BFREE(conn);
    return 0;
}

#endif	/* CONN_MODEL_H */

