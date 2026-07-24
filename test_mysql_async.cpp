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
    
    // Test non-blocking connect
    MYSQL *ret = NULL;
    const char *password = std::getenv("DB_PASSWORD");
    int status = mysql_real_connect_start(&ret, conn, "127.0.0.1", "root", password ? password : "", "math_ai_review", 3306, NULL, 0);
    printf("mysql_real_connect_start returned: status=%d, ret=%p\n", status, (void*)ret);
    
    if (status == 0) {
        // Completed immediately
        if (ret) {
            printf("Connected successfully (immediate)!\n");
            mysql_close(ret);
        } else {
            printf("Connection failed: %s\n", mysql_error(conn));
        }
        return 0;
    }
    
    // Need to continue
    while (status != 0) {
        // Wait for socket activity (simplified - just poll)
        status = mysql_real_connect_cont(&ret, conn, status);
        printf("mysql_real_connect_cont: status=%d\n", status);
        if (status == 0) {
            if (ret) {
                printf("Connected successfully (async)!\n");
                mysql_close(ret);
            } else {
                printf("Connection failed: %s\n", mysql_error(conn));
            }
            break;
        }
    }
    
    return 0;
}
