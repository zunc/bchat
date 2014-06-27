/* 
 * File:   room.h
 * Author: khoai
 *
 * Created on April 8, 2014, 11:44 AM
 */

// <!> this module currently development ^-^

#ifndef ROOM_H
#define	ROOM_H

#include "common.h"
#include "dbg.h"
#include "uthash.h"
#include "connection.h"
#include "util.h"

typedef struct {
    connection *conn;
    int id;
    UT_hash_handle hh;
} users_conn;

typedef struct {
    int index;
    enum STATE state;
    long lastActive;
    uint8_t *name;
    uint8_t is_pass; // 1 pass,  0 none-pass
    uint8_t *password; // password for access room
    uint8_t *admin_password; // password of admin
    uint32_t admin_id;

    int id;
    UT_hash_handle hh;

    users_conn *users;
} room;

uint32_t idRoomZen = 0;
room *room_hash = NULL;

room* room_head() {
    return room_hash;
}

uint32_t room_count() {
    return HASH_COUNT(room_hash);
}

room* room_add(uint8_t* name, uint32_t len) {
    room *rm = malloc(sizeof (room));
    uint32_t id = 0;
    uint32_t hash = 0;

    //debug("add: %d", fd);

    if (!rm) {
        log_warn("free error");
        return NULL;
    }

    hash = jenkins_hash(name, len);

    ATOMIC_INCREASE(&idRoomZen);
    rm->id = hash;
    rm->lastActive = 0;
    rm->state = UNINIT;
    rm->name = malloc(len);
    rm->is_pass = 0;
    memcpy(rm->name, name, len);

    HASH_ADD_INT(room_hash, id, rm);

    //log_info("count: %d", connmap_count());
    return rm;
}

room* room_get(uint8_t* name, uint32_t len) {
    room *rm = NULL;
    uint32_t hash = jenkins_hash(name, len);
    HASH_FIND_INT(room_hash, &hash, rm);
    return rm;
}

room* room_get_by_id(uint32_t hash) {
    room *rm = NULL;
    HASH_FIND_INT(room_hash, &hash, rm);
    return rm;
}

void room_remove(room* rm) {
    HASH_DEL(room_hash, rm);
}

//--- conn-room

int room_join(uint8_t* name, uint32_t len, connection *conn) {
    enum USER_PERMISSTION permission = USER;
    room *rm = room_get(name, len);
    room *new_room = NULL;
    users_conn *user = NULL;

    // create room when which wasn't exits
    if (!rm) {
        permission = ADMIN;
        new_room = room_add(name, len);
        if (!new_room) {
            log_warn("new_room == NULL");
            return -1;
        }

        new_room->admin_id = conn->id;
        new_room->lastActive = 0;
        new_room->name = malloc(len);
        memcpy(new_room->name, name, len);
        new_room->users = NULL;
        rm = new_room;
    }

    // set info on conn
    conn->room_session = rm->id;
    conn->permission = permission;

    // set info
    user = malloc(sizeof (users_conn));
    user->conn = conn;
    user->id = conn->fd;
    HASH_ADD_INT(new_room->users, conn->fd, user);
    log_info("[-] [%s] join [%s]", conn->user, new_room->name);
    return permission;
}

int room_left(connection *conn) {
    room *rm = NULL;
    users_conn *user = NULL;

    if (!conn) {
        log_warn("conn == NULL");
        return -1;
    }

    // get room
    rm = room_get_by_id(conn->room_session);
    if (rm) {
        // get user
        HASH_FIND_INT(room_hash->users, &conn->fd, user);
        if (user) {
            //			HASH_DEL(room_hash->users, user);
            log_info("[-] [%s] left [%s]", conn->user, rm->name);

            // set info
            conn->room_session = 0;
            conn->permission = UNSET;
        }
    }
    return 0;
}

#endif	/* ROOM_H */

