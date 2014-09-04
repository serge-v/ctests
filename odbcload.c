#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

void print_error(SQLSMALLINT handle_type, SQLHANDLE handle)
{
    SQLINTEGER    i, native, r;
    SQLCHAR state [7];
    SQLCHAR text  [2048];
    SQLSMALLINT   len;

    i = 0;
    do
    {
        r = SQLGetDiagRec(handle_type, handle, ++i, state, &native, text, sizeof(text), &len );
        if( SQL_SUCCEEDED( r ))
        {
            printf("\n%d. %s[%d] %s\n", i, state, native, text);
        }
    }
    while (r == SQL_SUCCESS);
}

int read_text(const char* fname, char** text)
{
    struct stat st;
    FILE* f = NULL;
    char* buff = NULL;
    int ret = -1;
    
    if (stat(fname, &st) != 0)
    {
        printf("Cannot stat file %s. Error: %d\n", fname, errno);
        goto out;
    }

    f = fopen(fname, "rt");
    if (!f)
    {
        printf("Cannot open file %s. Error: %d\n", fname, errno);
        goto out;
    }
    
    buff = malloc(st.st_size + 1);
    if (!buff)
    {
        printf("Cannot allocate %lu bytes. Error: %d\n", st.st_size + 1, errno);
        goto out;
    }
    
    fread(buff, 1, st.st_size, f);
    fclose(f);
    f = NULL;
    buff[st.st_size] = 0;
    *text = buff;
    ret = 0;
    
out:
    if (f)
        fclose(f);
        
    return ret;
}

int main(int argc, char** argv)
{
    SQLHENV henv = NULL;
    SQLHDBC hdbc = NULL;
    SQLHSTMT hstmt = NULL;
    SQLRETURN retcode = 0;
    int ret = 1;
    const char* connstr = NULL;
    char* text = NULL;
    const char* fname = NULL;
    int columns = 0;
    
    if (argc < 2)
    {
        printf("Usage: odbcload -c \"CONNSTR\" -f FILE\n");
        goto out;
    }
    
    int i;
    for (i = 1; i < argc; i++)
    {
        if (0 == strcmp(argv[i], "-c"))
        {
            if (i + 1 < argc)
                connstr = argv[i + 1];
        }
        else if (0 == strcmp(argv[i], "-f"))
        {
            if (i + 1 < argc)
                fname = argv[i + 1];
        }
    }
    
    if (!fname)
    {
        printf("File is not specified. Use -f FILE option\n");
        goto out;
    }
    
    if (!connstr)
    {
        printf("Connection string is not specified. Use -c \"CONNSTR\" option\n");
        goto out;
    }
    
    if (read_text(fname, &text) != 0)
        goto out;
    
    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    if (!SQL_SUCCEEDED(retcode))
    {
        printf("SQLAllocHandle SQL_HANDLE_ENV: %d\n", retcode);
        goto out;
    }
   
    retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0); 
    if (!SQL_SUCCEEDED(retcode))
    {
        printf("SQLAllocHandle SQL_ATTR_ODBC_VERSION: %d\n", retcode);
        goto out;
    }

    retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc); 
    if (!SQL_SUCCEEDED(retcode))
    {
        printf("SQLAllocHandle SQL_HANDLE_DBC: %d\n", retcode);
        print_error(SQL_HANDLE_ENV, henv);
        goto out;
    }
    
    retcode = SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);
    if (!SQL_SUCCEEDED(retcode))
    {
        printf("SQLSetConnectAttr: %d\n", retcode);
        print_error(SQL_HANDLE_DBC, hdbc);
        goto out;
    }

    retcode = SQLDriverConnect(hdbc, NULL, (SQLCHAR*)connstr, strlen(connstr), NULL, 0, NULL, SQL_DRIVER_NOPROMPT );
    if (!SQL_SUCCEEDED(retcode))
    {
        printf("SQLDriverConnect: %d\n", retcode);
        print_error(SQL_HANDLE_DBC, hdbc);
        goto out;
    }
    
    retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt); 
    if (!SQL_SUCCEEDED(retcode))
    {
        printf("SQLAllocHandle: %d\n", retcode);
        print_error(SQL_HANDLE_DBC, hdbc);
        goto out;
    }

    retcode = SQLExecDirect(hstmt, (SQLCHAR*)text, SQL_NTS);
    if (!SQL_SUCCEEDED(retcode))
    {
        printf("SQLExecDirect: %d\n", retcode);
        print_error(SQL_HANDLE_STMT, hstmt);
        goto out;
    }
    
    SQLSMALLINT retval = 3;
    retcode = SQLNumResultCols(hstmt, &retval);
    if (!SQL_SUCCEEDED(retcode) || retval == 0)
    {
        printf("No results\n");
        goto out;
    }
    
    columns = retval;

    while (1)
    {
        retcode = SQLFetch(hstmt);
        
        if (retcode == SQL_ERROR)
        {
            printf("SQLFetch: %d\n", retcode);
            print_error(SQL_HANDLE_STMT, hstmt);
            goto out;
        }
        
        if (retcode != SQL_SUCCESS)
            break;

        for (i = 1; i <= columns; i++)
        {
            SQLCHAR buf[50];
            SQLINTEGER ind = 0;
            
            // !!! SQLGetData causes memory corruption at retval address .
	    // TODO: try change SQLINTEGER to uint64_t
            retcode = SQLGetData(hstmt, i, SQL_C_CHAR, buf, sizeof(buf), &ind);

            if (ind == SQL_NULL_DATA)
                strcpy((char*)buf, "NULL");
            if (!SQL_SUCCEEDED(retcode))
            {
                printf("SQLGetData: %d\n", retcode);
                print_error(SQL_HANDLE_STMT, hstmt);
                goto out;
            }
            printf("%s", buf);
            if (i < columns)
                printf("\t");
        }
        printf("\n");
    }
    
    ret = 0;

out:

    if (text)
        free(text);

    if (hstmt)
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        
    if (hdbc)
    {
        SQLDisconnect(hdbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
    }
    
    if (henv)
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
   
    return ret;
}
