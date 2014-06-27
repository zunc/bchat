/* 
 * File:   util.h
 * Author: khoai
 *
 * Created on April 3, 2014, 2:41 PM
 */

#ifndef UTIL_H
#define	UTIL_H

// ref: http://en.wikipedia.org/wiki/Jenkins_hash_function

uint32_t jenkins_hash(uint8_t *key, size_t len) {
    uint32_t hash, i;
    for (hash = i = 0; i < len; ++i) {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

#define TOKEN "\r\n"

uint32_t stardard_message(uint8_t *message, size_t len) {
    // <!> fix it, hard parser
    message = strtok(message, TOKEN);
    if (!message)
        return 0;
    return strlen(message);
}

uint32_t stardard_user_name(uint8_t *message, size_t len) {
    // <!> last character much be null
    uint8_t* pos = message;
    uint8_t* last = message + len - 1;
    while (1) {
        if ((*pos == '\n') || (*pos == '\r')) {
            *pos = 0;
            break;
        }

        if (pos >= last) {
            break;
        }
        pos++;
    }
    return (pos - message);
}

#endif	/* UTIL_H */

