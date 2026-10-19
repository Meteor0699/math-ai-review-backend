// Test the MariaDB DLL's native async functions (without our wrapper)
#include <mysql.h>
#include <cstdio>
#include <cstdlib>

int main() {
    fprintf(stderr, "Testing DLL native async...\n");
    MYSQL *conn = mysql_init(NULL);
    if (!conn) {
        fprintf(stderr, "mysql_init failed\n");
        return 1;
    }
    
    // Set non-blocking mode
    my_bool nb = 1;
    mysql_options(conn, MYSQL_OPT_NONBLOCK, &nb);
    
    MYSQL *ret = NULL;
    const char *password = std::getenv("DB_PASSWORD");
    int status = mysql_real_connect_start(&ret, conn, "127.0.0.1", "root", password ? password : "", "math_ai_review", 3306, NULL, 0);
    fprintf(stderr, "mysql_real_connect_start status=%d, ret=%p\n", status, (void*)ret);
    
    if (status == 0) {
        fprintf(stderr, "Connected immediately! Server: %s\n", mysql_get_server_info(ret));
        mysql_close(ret);
    } else {
        fprintf(stderr, "Need to wait, error: %s\n", mysql_error(conn));
        mysql_close(conn);
    }
    return 0;
}
