/* 
 * File:   connmap.h
 * Author: khoai
 *
 * Created on April 3, 2014, 3:49 PM
 */

#ifndef CONNMAP_H
#define	CONNMAP_H

//#include "uthash.h"
#include "connection.h"
#include "common.h"
#include "dbg.h"

typedef struct {
    uint32_t total_conn;
} conmap_stats;

uint32_t idZen = 0;
connection *conn_hash = NULL;

connection* conmap_get() {
    return conn_hash;
}

uint32_t connmap_count() {
    return HASH_COUNT(conn_hash);
}

connection* connmap_add(int fd) {
    connection *conn = malloc(sizeof (connection));
    //debug("add: %d", fd);

    if (!conn) {
        log_warn("free error");
        return NULL;
    }
    ATOMIC_INCREASE(&idZen);
    conn->fd = fd;
    conn->parentId = ATOMIC_READ(&idZen);
    conn->lastActive = 0;
    conn->state = UNINIT;
    conn->id = fd;

    HASH_ADD_INT(conn_hash, fd, conn);

    //log_info("count: %d", connmap_count());
    return conn;
}

connection* connmap_get(int fd) {
    connection *con = NULL;
    HASH_FIND_INT(conn_hash, &fd, con);
    return con;
}

void connmap_remove(connection* conn) {
    HASH_DEL(conn_hash, conn);
}

#endif	/* CONNMAP_H */

