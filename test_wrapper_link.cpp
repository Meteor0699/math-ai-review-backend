#include <mysql.h>
#include <cstdio>
#include <cstdlib>

// Test that our wrapper functions are called instead of the DLL's async functions
int STDCALL mysql_real_connect_start(MYSQL **ret, MYSQL *mysql,
                                     const char *host,
                                     const char *user,
                                     const char *passwd,
                                     const char *db,
                                     unsigned int port,
                                     const char *unix_socket,
                                     unsigned long clientflag);

int main() {
    fprintf(stderr, "Testing wrapper...\n");
    MYSQL *conn = mysql_init(NULL);
    if (!conn) {
        fprintf(stderr, "mysql_init failed\n");
        return 1;
    }
    
    MYSQL *ret = NULL;
    int result = mysql_real_connect_start(&ret, conn, "127.0.0.1", "root", "123456", "math_ai_review", 3306, NULL, 0);
    fprintf(stderr, "mysql_real_connect_start result=%d, ret=%p\n", result, (void*)ret);
    
    if (ret) {
        fprintf(stderr, "Connection OK, server version: %s\n", mysql_get_server_info(ret));
        mysql_close(ret);
    } else {
        fprintf(stderr, "Connection failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return 1;
    }
    return 0;
}