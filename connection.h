/* 
 * File:   connection.h
 * Author: khoai
 *
 * Created on April 3, 2014, 3:27 PM
 */

#ifndef CONNECTION_H
#define	CONNECTION_H

#include "uthash.h"
#include "dbg.h"
#include "message.h"
#include "utstring.h"
#include "common.h"

//#include "connmap.h"

enum STATE {
    UNINIT,
    AUTHEN,
    DIE
};

enum USER_PERMISSTION {
    USER,
    ADMIN,
    UNSET
};

typedef struct {
    int fd;
    enum STATE state;
    long lastActive;
    uint32_t parentId;
    uint8_t* user;
    char ip[20];

    uint32_t room_session; // 0 is public room
    enum USER_PERMISSTION permission;

    int id; /* we'll use this field as the key */
    UT_hash_handle hh; /* makes this structure hashable */
} connection;

typedef int (*new_conn_cb)(int fd);
typedef int (*read_cb)(connection* conn, uint8_t *data, uint32_t len);
typedef int (*write_cb)(connection* conn);
typedef int (*close_cb)(connection* conn);

typedef struct {
    read_cb on_read;
    write_cb on_write;
    close_cb on_close;
    new_conn_cb on_new_conn;
} conn_handler;

conn_handler conn_callback;

void set_handler(read_cb _cb_read, write_cb _cb_write, close_cb _cb_close,
        new_conn_cb _cb_new_conn) {
    conn_callback.on_read = _cb_read;
    conn_callback.on_write = _cb_write;
    conn_callback.on_close = _cb_close;
    conn_callback.on_new_conn = _cb_new_conn;
}

int conn_read(connection* conn) {
    int is_done = 0;
    if (!conn) {
        log_warn("conn_read: conn == NULL");
        return 1;
    }

    while (1) {
        ssize_t count;
        uint8_t buf[512] = {0};

        count = read(conn->fd, buf, sizeof buf);

        if (count == -1) {
            /* If errno == EAGAIN, that means we have read all
             * data. So go back to the main loop. */
            if (errno != EAGAIN) {
                perror("read");
                is_done = 1;
            }
            break;
        } else if (count == 0) {
            /* End of file. The remote has closed the
             * connection. */
            is_done = 1;
            break;
        }

        //		state_control(conn, buf, count);
        if (conn_callback.on_read) {
            conn_callback.on_read(conn, buf, count);
        }

        //		conn_write(conn, buf, count);
        //		break;
    }

    if (is_done) {
        // close(conn->fd);
        // conn_close(conn);
        return -1;
    }
}

int conn_write(connection* conn, uint8_t* data, uint32_t len) {
    if (!conn) {
        log_warn("conn_write: conn == NULL");
        return 1;
    }

    if (write(conn->fd, data, len) < 0) {
        log_warn("write_err: fd=%d", conn->fd);
    }

    if (conn_callback.on_write) {
        conn_callback.on_write(conn);
    }
}

int conn_close(connection* conn) {
    if (!conn) {
        log_warn("conn_close: conn == NULL");
        return 1;
    }
    close(conn->fd);
    conn->state = DIE;
    BFREE(conn->user);
}

#endif	/* CONNECTION_H */

