#include <mysql.h>
#include <cstdio>
#include <cstdlib>

int main() {
    MYSQL *conn = mysql_init(NULL);
    if (!conn) {
        printf("mysql_init failed\n");
        return 1;
    }
    
    unsigned long timeout = 10;
    mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    
    const char *password = std::getenv("DB_PASSWORD");
    if (mysql_real_connect(conn, "127.0.0.1", "root", password ? password : "", "math_ai_review", 3306, NULL, 0)) {
        printf("Connected successfully with mysql_real_connect!\n");
        mysql_close(conn);
        return 0;
    } else {
        printf("mysql_real_connect failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return 1;
    }
}
