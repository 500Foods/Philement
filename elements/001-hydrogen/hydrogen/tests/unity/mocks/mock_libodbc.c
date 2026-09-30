/*
 * Mock libodbc functions for unit testing
 *
 * This file provides mock implementations of libodbc (unixODBC) functions
 * to enable testing of MSSQL database operations.
 * It mirrors mock_libdb2 but is mssql-owned so DB2 tests do not collide.
 * All function names are prefixed with mssql_mock_ to avoid symbol collision.
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

// SQL constants
#define SQL_SUCCESS 0

// Include the header but undefine the macros to access real functions if needed
#include "mock_libodbc.h"

// Undefine the macros in this file so we can declare the functions
#undef SQLAllocHandle_ptr
#undef SQLConnect_ptr
#undef SQLDriverConnect_ptr
#undef SQLExecDirect_ptr
#undef SQLFetch_ptr
#undef SQLGetData_ptr
#undef SQLNumResultCols_ptr
#undef SQLRowCount_ptr
#undef SQLFreeHandle_ptr
#undef SQLDisconnect_ptr
#undef SQLEndTran_ptr
#undef SQLPrepare_ptr
#undef SQLExecute_ptr
#undef SQLFreeStmt_ptr
#undef SQLDescribeCol_ptr
#undef SQLGetDiagRec_ptr
#undef SQLSetConnectAttr_ptr
#undef SQLBindParameter_ptr
#undef SQLCancel_ptr

// Function prototypes - must match types.h exactly
int mssql_mock_SQLAllocHandle(int handleType, void* inputHandle, void** outputHandle);
int mssql_mock_SQLConnect(void* connectionHandle, char* serverName, int nameLength,
                   char* userName, int userLength, char* password, int passwordLength);
int mssql_mock_SQLDriverConnect(void* connectionHandle, void* windowHandle, unsigned char* connectionString,
                          short stringLength, unsigned char* outConnectionString, short bufferLength,
                          short* stringLengthPtr, unsigned short driverCompletion);
int mssql_mock_SQLExecDirect(void* statementHandle, char* statementText, int textLength);
int mssql_mock_SQLFetch(void* statementHandle);
int mssql_mock_SQLGetData(void* statementHandle, int columnNumber, int targetType,
                    void* targetValue, int bufferLength, int* strLenOrIndPtr);
int mssql_mock_SQLNumResultCols(void* statementHandle, int* columnCount);
int mssql_mock_SQLRowCount(void* statementHandle, int* rowCount);
int mssql_mock_SQLFreeHandle(int handleType, void* handle);
int mssql_mock_SQLDisconnect(void* connectionHandle);
int mssql_mock_SQLEndTran(int handleType, void* handle, int completionType);
int mssql_mock_SQLPrepare(void* statementHandle, unsigned char* statementText, int textLength);
int mssql_mock_SQLExecute(void* statementHandle);
int mssql_mock_SQLFreeStmt(void* statementHandle, int option);
int mssql_mock_SQLDescribeCol(void* statementHandle, int columnNumber, unsigned char* columnName,
                        int bufferLength, short* nameLength, int* dataType, int* columnSize,
                        short* decimalDigits, short* nullable);
int mssql_mock_SQLGetDiagRec(short handleType, void* handle, short recNumber, unsigned char* sqlState,
                       long* nativeError, unsigned char* messageText, short bufferLength, short* textLength);

int mssql_mock_SQLSetEnvAttr(void* environmentHandle, int attribute, long* value, int stringLength);

void mssql_mock_libodbc_set_SQLAllocHandle_result(int result);
void mssql_mock_libodbc_set_SQLAllocHandle_output_handle(void* handle);
void mssql_mock_libodbc_set_SQLDriverConnect_result(int result);
void mssql_mock_libodbc_set_SQLExecDirect_result(int result);
void mssql_mock_libodbc_set_SQLFreeHandle_result(int result);
void mssql_mock_libodbc_set_SQLEndTran_result(int result);
void mssql_mock_libodbc_reset_all(void);

// Static variables to store mock state
static int mssql_mock_SQLAllocHandle_result = 0; // 0 = SQL_SUCCESS
static void* mssql_mock_SQLAllocHandle_output_handle = (void*)0x12345678; // Mock handle
static int mssql_mock_SQLDriverConnect_result = 0; // 0 = SQL_SUCCESS
static int mssql_mock_SQLExecDirect_result = 0; // 0 = SQL_SUCCESS
static int mssql_mock_SQLExecute_result = 0; // 0 = SQL_SUCCESS
static int mssql_mock_SQLFetch_result = 0; // 0 = SQL_SUCCESS
static int mssql_mock_fetch_row_count = 0; // Number of rows to return before SQL_NO_DATA
static int mssql_mock_fetch_current_row = 0; // Current row counter
static int mssql_mock_SQLNumResultCols_result = 0; // 0 = SQL_SUCCESS
static int mssql_mock_SQLNumResultCols_column_count = 1;
static int mssql_mock_SQLRowCount_result = 0; // 0 = SQL_SUCCESS
static int mssql_mock_SQLRowCount_row_count = 1;
static int mssql_mock_SQLDescribeCol_result = 0; // 0 = SQL_SUCCESS
static char mssql_mock_SQLDescribeCol_column_name[256] = "test_column";
static int mssql_mock_SQLGetData_result = 0; // 0 = SQL_SUCCESS
static char mssql_mock_SQLGetData_data[4096] = "test_data";
static int mssql_mock_SQLGetData_data_len = 9; // strlen("test_data")
static int mssql_mock_SQLGetDiagRec_result = 0; // 0 = SQL_SUCCESS
static char mssql_mock_SQLGetDiagRec_sqlstate[6] = "42000";
static long mssql_mock_SQLGetDiagRec_native_error = 12345;
static char mssql_mock_SQLGetDiagRec_message[1024] = "Mock error message";
static int mssql_mock_SQLFreeHandle_result = 0; // 0 = SQL_SUCCESS
static int mssql_mock_SQLEndTran_result = 0; // 0 = SQL_SUCCESS
static int mssql_mock_SQLSetConnectAttr_result = 0; // 0 = SQL_SUCCESS
static int mssql_mock_SQLPrepare_result = 0; // 0 = SQL_SUCCESS
static int mssql_mock_SQLBindParameter_result = 0; // 0 = SQL_SUCCESS

static int mssql_mock_SQLSetConnectAttr_last_attribute = -1;
static long mssql_mock_SQLSetConnectAttr_last_value = 0;
static int mssql_mock_SQLSetConnectAttr_call_count = 0;
static int mssql_mock_SQLSetConnectAttr_autocommit_off_seen = 0;

// Mock implementations
int mssql_mock_SQLAllocHandle(int handleType, void* inputHandle, void** outputHandle) {
    (void)handleType;
    (void)inputHandle;
    if (outputHandle) {
        *outputHandle = mssql_mock_SQLAllocHandle_output_handle;
    }
    return mssql_mock_SQLAllocHandle_result;
}

int mssql_mock_SQLConnect(void* connectionHandle, char* serverName, int nameLength,
                   char* userName, int userLength, char* password, int passwordLength) {
    (void)connectionHandle;
    (void)serverName;
    (void)nameLength;
    (void)userName;
    (void)userLength;
    (void)password;
    (void)passwordLength;
    return 0; // Always success for now
}

int mssql_mock_SQLDriverConnect(void* connectionHandle, void* windowHandle, unsigned char* connectionString,
                          short stringLength, unsigned char* outConnectionString, short bufferLength,
                          short* stringLengthPtr, unsigned short driverCompletion) {
    (void)connectionHandle;
    (void)windowHandle;
    (void)connectionString;
    (void)stringLength;
    (void)outConnectionString;
    (void)bufferLength;
    (void)stringLengthPtr;
    (void)driverCompletion;
    return mssql_mock_SQLDriverConnect_result;
}

int mssql_mock_SQLExecDirect(void* statementHandle, char* statementText, int textLength) {
    (void)statementHandle;
    (void)statementText;
    (void)textLength;
    // Reset fetch counter on new query
    mssql_mock_fetch_current_row = 0;
    return mssql_mock_SQLExecDirect_result;
}

int mssql_mock_SQLFetch(void* statementHandle) {
    (void)statementHandle;

    // Return rows up to the configured count, then SQL_NO_DATA (100)
    if (mssql_mock_fetch_row_count == 0 || mssql_mock_fetch_current_row >= mssql_mock_fetch_row_count) {
        return 100; // SQL_NO_DATA
    }

    mssql_mock_fetch_current_row++;
    return mssql_mock_SQLFetch_result;
}

int mssql_mock_SQLGetData(void* statementHandle, int columnNumber, int targetType,
                    void* targetValue, int bufferLength, int* strLenOrIndPtr) {
    (void)statementHandle;
    (void)columnNumber;
    (void)targetType;

    if (mssql_mock_SQLGetData_result != 0) {
        return mssql_mock_SQLGetData_result;
    }

    if (targetValue && bufferLength > 0) {
        size_t data_len = strlen(mssql_mock_SQLGetData_data);
        size_t copy_len = (data_len < (size_t)bufferLength - 1) ? data_len : (size_t)bufferLength - 1;
        memcpy(targetValue, mssql_mock_SQLGetData_data, copy_len);
        ((char*)targetValue)[copy_len] = '\0';
    }

    if (strLenOrIndPtr) {
        *strLenOrIndPtr = mssql_mock_SQLGetData_data_len;
    }

    return 0; // SQL_SUCCESS
}

int mssql_mock_SQLNumResultCols(void* statementHandle, int* columnCount) {
    (void)statementHandle;
    if (columnCount) {
        *columnCount = mssql_mock_SQLNumResultCols_column_count;
    }
    return mssql_mock_SQLNumResultCols_result;
}

int mssql_mock_SQLRowCount(void* statementHandle, int* rowCount) {
    (void)statementHandle;
    if (rowCount) {
        *rowCount = mssql_mock_SQLRowCount_row_count;
    }
    return mssql_mock_SQLRowCount_result;
}

int mssql_mock_SQLFreeHandle(int handleType, void* handle) {
    (void)handleType;
    (void)handle;
    return mssql_mock_SQLFreeHandle_result;
}

int mssql_mock_SQLDisconnect(void* connectionHandle) {
    (void)connectionHandle;
    return 0; // Always success
}

int mssql_mock_SQLEndTran(int handleType, void* handle, int completionType) {
    (void)handleType;
    (void)handle;
    (void)completionType;
    return mssql_mock_SQLEndTran_result;
}

int mssql_mock_SQLPrepare(void* statementHandle, unsigned char* statementText, int textLength) {
    (void)statementHandle;
    (void)statementText;
    (void)textLength;
    return mssql_mock_SQLPrepare_result;
}

int mssql_mock_SQLExecute(void* statementHandle) {
    (void)statementHandle;
    // Reset fetch counter on new execution
    mssql_mock_fetch_current_row = 0;
    return mssql_mock_SQLExecute_result;
}

int mssql_mock_SQLFreeStmt(void* statementHandle, int option) {
    (void)statementHandle;
    (void)option;
    return 0; // Always success
}

int mssql_mock_SQLDescribeCol(void* statementHandle, int columnNumber, unsigned char* columnName,
                         int bufferLength, short* nameLength, int* dataType, int* columnSize,
                         short* decimalDigits, short* nullable) {
    (void)statementHandle;
    (void)columnNumber;
    (void)dataType;
    (void)columnSize;
    (void)decimalDigits;
    (void)nullable;

    if (mssql_mock_SQLDescribeCol_result != 0) {
        return mssql_mock_SQLDescribeCol_result;
    }

    if (columnName && bufferLength > 0) {
        size_t len = (size_t)(bufferLength - 1);
        strncpy((char*)columnName, mssql_mock_SQLDescribeCol_column_name, len);
        ((char*)columnName)[len] = '\0';
    }

    if (nameLength) {
        *nameLength = (short)strlen(mssql_mock_SQLDescribeCol_column_name);
    }

    return 0; // SQL_SUCCESS
}

int mssql_mock_SQLGetDiagRec(short handleType, void* handle, short recNumber, unsigned char* sqlState,
                       long* nativeError, unsigned char* messageText, short bufferLength, short* textLength) {
    (void)handleType;
    (void)handle;
    (void)recNumber;

    if (mssql_mock_SQLGetDiagRec_result != 0) {
        return mssql_mock_SQLGetDiagRec_result;
    }

    if (sqlState) {
        strncpy((char*)sqlState, mssql_mock_SQLGetDiagRec_sqlstate, 5);
        ((char*)sqlState)[5] = '\0';
    }

    if (nativeError) {
        *nativeError = mssql_mock_SQLGetDiagRec_native_error;
    }

    if (messageText && bufferLength > 0) {
        size_t len = (size_t)(bufferLength - 1);
        strncpy((char*)messageText, mssql_mock_SQLGetDiagRec_message, len);
        ((char*)messageText)[len] = '\0';
    }

    if (textLength) {
        *textLength = (short)strlen(mssql_mock_SQLGetDiagRec_message);
    }

    return 0; // SQL_SUCCESS
}

int mssql_mock_SQLSetConnectAttr(void* connectionHandle, int attribute, long value, int stringLength) {
    (void)connectionHandle;
    (void)stringLength;
    mssql_mock_SQLSetConnectAttr_call_count++;
    mssql_mock_SQLSetConnectAttr_last_attribute = attribute;
    mssql_mock_SQLSetConnectAttr_last_value = value;
    /* SQL_ATTR_AUTOCOMMIT=102, SQL_AUTOCOMMIT_OFF=0 */
    if (attribute == 102 && value == 0) {
        mssql_mock_SQLSetConnectAttr_autocommit_off_seen = 1;
    }
    return mssql_mock_SQLSetConnectAttr_result;
}

int mssql_mock_SQLBindParameter(void* statementHandle, unsigned short parameterNumber, short inputOutputType,
                           short valueType, short parameterType, unsigned long columnSize, short decimalDigits,
                           void* parameterValue, long bufferLength, long* strLenOrIndPtr) {
    (void)statementHandle;
    (void)parameterNumber;
    (void)inputOutputType;
    (void)valueType;
    (void)parameterType;
    (void)columnSize;
    (void)decimalDigits;
    (void)parameterValue;
    (void)bufferLength;
    (void)strLenOrIndPtr;
    return mssql_mock_SQLBindParameter_result;
}

int mssql_mock_SQLCancel(void* statementHandle) {
    (void)statementHandle;
    return 0; // Always success
}

int mssql_mock_SQLSetEnvAttr(void* environmentHandle, int attribute, long* value, int stringLength) {
    (void)environmentHandle;
    (void)attribute;
    (void)value;
    (void)stringLength;
    return 0; // Always success
}

// Mock control functions
void mssql_mock_libodbc_set_SQLAllocHandle_result(int result) {
    mssql_mock_SQLAllocHandle_result = result;
}

void mssql_mock_libodbc_set_SQLAllocHandle_output_handle(void* handle) {
    mssql_mock_SQLAllocHandle_output_handle = handle;
}

void mssql_mock_libodbc_set_SQLDriverConnect_result(int result) {
    mssql_mock_SQLDriverConnect_result = result;
}

void mssql_mock_libodbc_set_SQLExecDirect_result(int result) {
    mssql_mock_SQLExecDirect_result = result;
}

void mssql_mock_libodbc_set_SQLFreeHandle_result(int result) {
    mssql_mock_SQLFreeHandle_result = result;
}

void mssql_mock_libodbc_set_SQLEndTran_result(int result) {
    mssql_mock_SQLEndTran_result = result;
}

void mssql_mock_libodbc_set_SQLExecute_result(int result) {
    mssql_mock_SQLExecute_result = result;
}

void mssql_mock_libodbc_set_SQLFetch_result(int result) {
    mssql_mock_SQLFetch_result = result;
}

void mssql_mock_libodbc_set_fetch_row_count(int count) {
    mssql_mock_fetch_row_count = count;
    mssql_mock_fetch_current_row = 0;
}

void mssql_mock_libodbc_set_SQLNumResultCols_result(int result, int column_count) {
    mssql_mock_SQLNumResultCols_result = result;
    mssql_mock_SQLNumResultCols_column_count = column_count;
}

void mssql_mock_libodbc_set_SQLRowCount_result(int result, int row_count) {
    mssql_mock_SQLRowCount_result = result;
    mssql_mock_SQLRowCount_row_count = row_count;
}

void mssql_mock_libodbc_set_SQLDescribeCol_result(int result) {
    mssql_mock_SQLDescribeCol_result = result;
}

void mssql_mock_libodbc_set_SQLDescribeCol_column_name(const char* name) {
    if (name) {
        strncpy(mssql_mock_SQLDescribeCol_column_name, name, sizeof(mssql_mock_SQLDescribeCol_column_name) - 1);
        mssql_mock_SQLDescribeCol_column_name[sizeof(mssql_mock_SQLDescribeCol_column_name) - 1] = '\0';
    }
}

void mssql_mock_libodbc_set_SQLGetData_result(int result) {
    mssql_mock_SQLGetData_result = result;
}

void mssql_mock_libodbc_set_SQLGetData_data(const char* data, int data_len) {
    if (data) {
        strncpy(mssql_mock_SQLGetData_data, data, sizeof(mssql_mock_SQLGetData_data) - 1);
        mssql_mock_SQLGetData_data[sizeof(mssql_mock_SQLGetData_data) - 1] = '\0';
        mssql_mock_SQLGetData_data_len = data_len;
    }
}

void mssql_mock_libodbc_set_SQLGetDiagRec_result(int result) {
    mssql_mock_SQLGetDiagRec_result = result;
}

void mssql_mock_libodbc_set_SQLGetDiagRec_error(const char* sqlstate, long native_error, const char* message) {
    if (sqlstate) {
        strncpy(mssql_mock_SQLGetDiagRec_sqlstate, sqlstate, 5);
        mssql_mock_SQLGetDiagRec_sqlstate[5] = '\0';
    }
    mssql_mock_SQLGetDiagRec_native_error = native_error;
    if (message) {
        strncpy(mssql_mock_SQLGetDiagRec_message, message, sizeof(mssql_mock_SQLGetDiagRec_message) - 1);
        mssql_mock_SQLGetDiagRec_message[sizeof(mssql_mock_SQLGetDiagRec_message) - 1] = '\0';
    }
}

void mssql_mock_libodbc_set_SQLSetConnectAttr_result(int result) {
    mssql_mock_SQLSetConnectAttr_result = result;
}

int mssql_mock_libodbc_get_SQLSetConnectAttr_autocommit_off_seen(void) {
    return mssql_mock_SQLSetConnectAttr_autocommit_off_seen;
}

int mssql_mock_libodbc_get_SQLSetConnectAttr_call_count(void) {
    return mssql_mock_SQLSetConnectAttr_call_count;
}

void mssql_mock_libodbc_set_SQLPrepare_result(int result) {
    mssql_mock_SQLPrepare_result = result;
}

void mssql_mock_libodbc_set_SQLBindParameter_result(int result) {
    mssql_mock_SQLBindParameter_result = result;
}

void mssql_mock_libodbc_reset_all(void) {
    mssql_mock_SQLAllocHandle_result = 0;
    mssql_mock_SQLAllocHandle_output_handle = (void*)0x12345678;
    mssql_mock_SQLDriverConnect_result = 0;
    mssql_mock_SQLExecDirect_result = 0;
    mssql_mock_SQLExecute_result = 0;
    mssql_mock_SQLFetch_result = 0;
    mssql_mock_fetch_row_count = 0;
    mssql_mock_fetch_current_row = 0;
    mssql_mock_SQLNumResultCols_result = 0;
    mssql_mock_SQLNumResultCols_column_count = 1;
    mssql_mock_SQLRowCount_result = 0;
    mssql_mock_SQLRowCount_row_count = 1;
    mssql_mock_SQLDescribeCol_result = 0;
    strncpy(mssql_mock_SQLDescribeCol_column_name, "test_column", sizeof(mssql_mock_SQLDescribeCol_column_name) - 1);
    mssql_mock_SQLGetData_result = 0;
    strncpy(mssql_mock_SQLGetData_data, "test_data", sizeof(mssql_mock_SQLGetData_data) - 1);
    mssql_mock_SQLGetData_data_len = 9;
    mssql_mock_SQLGetDiagRec_result = 0;
    memcpy(mssql_mock_SQLGetDiagRec_sqlstate, "42000\0", 6);
    mssql_mock_SQLGetDiagRec_native_error = 12345;
    strncpy(mssql_mock_SQLGetDiagRec_message, "Mock error message", sizeof(mssql_mock_SQLGetDiagRec_message) - 1);
    mssql_mock_SQLFreeHandle_result = 0;
    mssql_mock_SQLEndTran_result = 0;
    mssql_mock_SQLSetConnectAttr_result = 0;
    mssql_mock_SQLSetConnectAttr_last_attribute = -1;
    mssql_mock_SQLSetConnectAttr_last_value = 0;
    mssql_mock_SQLSetConnectAttr_call_count = 0;
    mssql_mock_SQLSetConnectAttr_autocommit_off_seen = 0;
    mssql_mock_SQLPrepare_result = 0;
    mssql_mock_SQLBindParameter_result = 0;
}
