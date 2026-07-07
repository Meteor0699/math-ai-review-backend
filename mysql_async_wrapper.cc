// mysql_async_wrapper.cc - Implements all non-blocking MySQL async functions
// that are expected by Drogon but not available in MySQL 5.7's libmysql.
// This implementation wraps the synchronous API.
//
// KEY INSIGHT: Drogon sets MYSQL_OPT_NONBLOCK before calling mysql_real_connect_start.
// We establish the connection synchronously, then return MYSQL_WAIT_WRITE to trigger
// Drogon's event loop. A connected socket is writable, so the event loop fires
// immediately, calling handleEvent() which calls mysql_real_connect_cont() to complete.

#include "mysql_async_wrapper.h"

#include <mysql.h>

// MYSQL_WAIT_* constants for non-blocking API
#ifndef MYSQL_WAIT_READ
#define MYSQL_WAIT_READ     1
#endif
#ifndef MYSQL_WAIT_WRITE
#define MYSQL_WAIT_WRITE    2
#endif
#ifndef MYSQL_WAIT_EXCEPT
#define MYSQL_WAIT_EXCEPT   4
#endif
#ifndef MYSQL_WAIT_TIMEOUT
#define MYSQL_WAIT_TIMEOUT  8
#endif

// mysql_real_connect_start - wraps synchronous mysql_real_connect.
// Returns MYSQL_WAIT_TIMEOUT to trigger Drogon's loop to complete the connection
// immediately via handleTimeout() -> mysql_real_connect_cont(). On Windows the
// socket writable event may not fire reliably for this compatibility wrapper.
int STDCALL mysql_real_connect_start(MYSQL **ret, MYSQL *mysql,
                                     const char *host,
                                     const char *user,
                                     const char *passwd,
                                     const char *db,
                                     unsigned int port,
                                     const char *unix_socket,
                                     unsigned long clientflag) {
    if (!mysql || !ret) {
        return 1;
    }

    MYSQL *result = mysql_real_connect(mysql, host, user, passwd, db, port,
                                       unix_socket, clientflag);
    if (result) {
        *ret = result;
        return MYSQL_WAIT_TIMEOUT;
    } else {
        *ret = NULL;
        return 1;
    }
}

// mysql_real_connect_cont - continue async connect
int STDCALL mysql_real_connect_cont(MYSQL **ret, MYSQL *mysql, int status) {
    if (!mysql || !ret) {
        return 1;
    }
    *ret = mysql;
    return 0;
}

// mysql_real_query_start - wraps synchronous mysql_real_query
int STDCALL mysql_real_query_start(int *ret, MYSQL *mysql, const char *q, unsigned long length) {
    if (!mysql || !ret) {
        return 1;
    }
    *ret = mysql_real_query(mysql, q, length);
    return 0;
}

// mysql_real_query_cont - continue async query
int STDCALL mysql_real_query_cont(int *ret, MYSQL *mysql, int status) {
    return 0;
}

// mysql_store_result_start - start storing result
int STDCALL mysql_store_result_start(MYSQL_RES **ret, MYSQL *mysql) {
    if (!mysql || !ret) {
        return 1;
    }
    *ret = mysql_store_result(mysql);
    return 0;
}

// mysql_store_result_cont - continue storing result
int STDCALL mysql_store_result_cont(MYSQL_RES **ret, MYSQL *mysql, int status) {
    return 0;
}

// mysql_next_result_start - start getting next result
int STDCALL mysql_next_result_start(int *ret, MYSQL *mysql) {
    if (!mysql || !ret) {
        return 1;
    }
    *ret = mysql_next_result(mysql);
    return 0;
}

// mysql_next_result_cont - continue getting next result
int STDCALL mysql_next_result_cont(int *ret, MYSQL *mysql, int status) {
    return 0;
}

// mysql_set_character_set_start - start setting character set
int STDCALL mysql_set_character_set_start(int *ret, MYSQL *mysql, const char *csname) {
    if (!mysql || !ret) {
        return 1;
    }
    *ret = mysql_set_character_set(mysql, csname);
    return 0;
}

// mysql_set_character_set_cont - continue setting character set
int STDCALL mysql_set_character_set_cont(int *ret, MYSQL *mysql, int status) {
    return 0;
}

// mysql_close_start - start closing connection
int STDCALL mysql_close_start(MYSQL *sock) {
    if (sock) {
        mysql_close(sock);
    }
    return 0;
}

// mysql_close_cont - continue closing connection
int STDCALL mysql_close_cont(MYSQL *sock, int status) {
    return 0;
}

// mysql_get_socket - returns the socket descriptor
my_socket STDCALL mysql_get_socket(MYSQL *mysql) {
    if (!mysql) {
        return -1;
    }
    return mysql->net.fd;
}

// mysql_get_timeout_value - returns zero so compatibility continuations run
// immediately when MYSQL_WAIT_TIMEOUT is used.
unsigned int STDCALL mysql_get_timeout_value(const MYSQL *mysql) {
    (void)mysql;
    return 0;
}

// mysql_get_timeout_value_ms - returns zero so compatibility continuations run
// immediately when MYSQL_WAIT_TIMEOUT is used.
unsigned int STDCALL mysql_get_timeout_value_ms(const MYSQL *mysql) {
    (void)mysql;
    return 0;
}
