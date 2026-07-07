// mysql_async_wrapper.c - Proxy DLL wrapper for libmariadb.dll
// Provides non-blocking functions (mysql_real_connect_start, mysql_real_connect_cont)
// using the synchronous API from libmariadb_core.dll

#include <windows.h>
#include <mysql.h>

// Function pointers to the real DLL
static HMODULE hRealDll = NULL;
static MYSQL* (STDCALL *real_mysql_real_connect)(MYSQL*, const char*, const char*, const char*, const char*, unsigned int, const char*, unsigned long) = NULL;

static void load_real_dll() {
    if (hRealDll) return;
    hRealDll = LoadLibraryA("libmariadb_core.dll");
    if (hRealDll) {
        real_mysql_real_connect = (void*)GetProcAddress(hRealDll, "mysql_real_connect");
    }
}

// mysql_real_connect_start - wrap synchronous API
int STDCALL mysql_real_connect_start(MYSQL **ret, MYSQL *mysql,
                                      const char *host, const char *user,
                                      const char *passwd, const char *db,
                                      unsigned int port, const char *unix_socket,
                                      unsigned long clientflag) {
    load_real_dll();
    if (!mysql || !ret || !real_mysql_real_connect) {
        if (ret) *ret = NULL;
        return -1;
    }
    
    MYSQL *result = real_mysql_real_connect(mysql, host, user, passwd, db, port, unix_socket, clientflag);
    if (result) {
        *ret = result;
        return 0;
    } else {
        *ret = NULL;
        return -1;
    }
}

// mysql_real_connect_cont - should never be called after start returns 0
int STDCALL mysql_real_connect_cont(MYSQL **ret, MYSQL *mysql, int status) {
    if (status == 0) {
        *ret = mysql;
        return 0;
    }
    *ret = NULL;
    return -1;
}