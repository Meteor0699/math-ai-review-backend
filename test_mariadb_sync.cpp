#include <mysql.h>
#include <cstdio>

int main() {
    fprintf(stderr, "Starting test...\n");
    MYSQL *conn = mysql_init(NULL);
    if (!conn) {
        fprintf(stderr, "mysql_init failed\n");
        return 1;
    }
    fprintf(stderr, "mysql_init OK, connecting...\n");
    unsigned int timeout = 10;
    mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    mysql_options(conn, MYSQL_OPT_READ_TIMEOUT, &timeout);
    if (mysql_real_connect(conn, "127.0.0.1", "root", "123456", "math_ai_review", 3306, NULL, 0)) {
        fprintf(stderr, "Connected OK! Server version: %s\n", mysql_get_server_info(conn));
        mysql_close(conn);
        return 0;
    } else {
        fprintf(stderr, "Connect failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return 1;
    }
}