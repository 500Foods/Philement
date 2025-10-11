/*
 * Mock libdb2 functions for unit testing
 *
 * This file provides mock implementations of libdb2 functions
 * to enable testing of DB2 database operations.
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

// Include the header but undefine the macros to access real functions if needed
#include "mock_libdb2.h"

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

// Function prototypes - must match types.h exactly
int mock_SQLAllocHandle(int handleType, void* inputHandle, void** outputHandle);
int mock_SQLConnect(void* connectionHandle, char* serverName, int nameLength,
                   char* userName, int userLength, char* password, int passwordLength);
int mock_SQLDriverConnect(void* connectionHandle, void* windowHandle, unsigned char* connectionString,
                         short stringLength, unsigned char* outConnectionString, short bufferLength,
                         short* stringLengthPtr, unsigned short driverCompletion);
int mock_SQLExecDirect(void* statementHandle, char* statementText, int textLength);
int mock_SQLFetch(void* statementHandle);
int mock_SQLGetData(void* statementHandle, int columnNumber, int targetType,
                   void* targetValue, int bufferLength, int* strLenOrIndPtr);
int mock_SQLNumResultCols(void* statementHandle, int* columnCount);
int mock_SQLRowCount(void* statementHandle, int* rowCount);
int mock_SQLFreeHandle(int handleType, void* handle);
int mock_SQLDisconnect(void* connectionHandle);
int mock_SQLEndTran(int handleType, void* handle, int completionType);
int mock_SQLPrepare(void* statementHandle, char* statementText, int textLength);
int mock_SQLExecute(void* statementHandle);
int mock_SQLFreeStmt(void* statementHandle, int option);
int mock_SQLDescribeCol(void* statementHandle, int columnNumber, unsigned char* columnName,
                       int bufferLength, short* nameLength, int* dataType, int* columnSize,
                       short* decimalDigits, short* nullable);
int mock_SQLGetDiagRec(short handleType, void* handle, short recNumber, unsigned char* sqlState,
                      long* nativeError, unsigned char* messageText, short bufferLength, short* textLength);

void mock_libdb2_set_SQLAllocHandle_result(int result);
void mock_libdb2_set_SQLAllocHandle_output_handle(void* handle);
void mock_libdb2_set_SQLDriverConnect_result(int result);
void mock_libdb2_set_SQLExecDirect_result(int result);
void mock_libdb2_set_SQLFreeHandle_result(int result);
void mock_libdb2_set_SQLEndTran_result(int result);
void mock_libdb2_reset_all(void);

// Static variables to store mock state
static int mock_SQLAllocHandle_result = 0; // 0 = SQL_SUCCESS
static void* mock_SQLAllocHandle_output_handle = (void*)0x12345678; // Mock handle
static int mock_SQLDriverConnect_result = 0; // 0 = SQL_SUCCESS
static int mock_SQLExecDirect_result = 0; // 0 = SQL_SUCCESS
static int mock_SQLExecute_result = 0; // 0 = SQL_SUCCESS
static int mock_SQLFetch_result = 0; // 0 = SQL_SUCCESS
static int mock_fetch_row_count = 0; // Number of rows to return before SQL_NO_DATA
static int mock_fetch_current_row = 0; // Current row counter
static int mock_SQLNumResultCols_result = 0; // 0 = SQL_SUCCESS
static int mock_SQLNumResultCols_column_count = 1;
static int mock_SQLRowCount_result = 0; // 0 = SQL_SUCCESS
static int mock_SQLRowCount_row_count = 1;
static int mock_SQLDescribeCol_result = 0; // 0 = SQL_SUCCESS
static char mock_SQLDescribeCol_column_name[256] = "test_column";
static int mock_SQLGetData_result = 0; // 0 = SQL_SUCCESS
static char mock_SQLGetData_data[256] = "test_data";
static int mock_SQLGetData_data_len = 9; // strlen("test_data")
static int mock_SQLGetDiagRec_result = 0; // 0 = SQL_SUCCESS
static char mock_SQLGetDiagRec_sqlstate[6] = "42000";
static long mock_SQLGetDiagRec_native_error = 12345;
static char mock_SQLGetDiagRec_message[1024] = "Mock error message";
static int mock_SQLFreeHandle_result = 0; // 0 = SQL_SUCCESS
static int mock_SQLEndTran_result = 0; // 0 = SQL_SUCCESS

// Mock implementations
int mock_SQLAllocHandle(int handleType, void* inputHandle, void** outputHandle) {
    (void)handleType;
    (void)inputHandle;
    if (outputHandle) {
        *outputHandle = mock_SQLAllocHandle_output_handle;
    }
    return mock_SQLAllocHandle_result;
}

int mock_SQLConnect(void* connectionHandle, char* serverName, int nameLength,
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

int mock_SQLDriverConnect(void* connectionHandle, void* windowHandle, unsigned char* connectionString,
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
    return mock_SQLDriverConnect_result;
}

int mock_SQLExecDirect(void* statementHandle, char* statementText, int textLength) {
    (void)statementHandle;
    (void)statementText;
    (void)textLength;
    // Reset fetch counter on new query
    mock_fetch_current_row = 0;
    return mock_SQLExecDirect_result;
}

int mock_SQLFetch(void* statementHandle) {
    (void)statementHandle;
    
    // Return rows up to the configured count, then SQL_NO_DATA (100)
    if (mock_fetch_row_count == 0 || mock_fetch_current_row >= mock_fetch_row_count) {
        return 100; // SQL_NO_DATA
    }
    
    mock_fetch_current_row++;
    return mock_SQLFetch_result;
}

int mock_SQLGetData(void* statementHandle, int columnNumber, int targetType,
                   void* targetValue, int bufferLength, int* strLenOrIndPtr) {
    (void)statementHandle;
    (void)columnNumber;
    (void)targetType;
    
    if (mock_SQLGetData_result != 0) {
        return mock_SQLGetData_result;
    }
    
    if (targetValue && bufferLength > 0) {
        size_t data_len = strlen(mock_SQLGetData_data);
        size_t copy_len = (data_len < (size_t)bufferLength - 1) ? data_len : (size_t)bufferLength - 1;
        memcpy(targetValue, mock_SQLGetData_data, copy_len);
        ((char*)targetValue)[copy_len] = '\0';
    }
    
    if (strLenOrIndPtr) {
        *strLenOrIndPtr = mock_SQLGetData_data_len;
    }
    
    return 0; // SQL_SUCCESS
}

int mock_SQLNumResultCols(void* statementHandle, int* columnCount) {
    (void)statementHandle;
    if (columnCount) {
        *columnCount = mock_SQLNumResultCols_column_count;
    }
    return mock_SQLNumResultCols_result;
}

int mock_SQLRowCount(void* statementHandle, int* rowCount) {
    (void)statementHandle;
    if (rowCount) {
        *rowCount = mock_SQLRowCount_row_count;
    }
    return mock_SQLRowCount_result;
}

int mock_SQLFreeHandle(int handleType, void* handle) {
    (void)handleType;
    (void)handle;
    return mock_SQLFreeHandle_result;
}

int mock_SQLDisconnect(void* connectionHandle) {
    (void)connectionHandle;
    return 0; // Always success
}

int mock_SQLEndTran(int handleType, void* handle, int completionType) {
    (void)handleType;
    (void)handle;
    (void)completionType;
    return mock_SQLEndTran_result;
}

int mock_SQLPrepare(void* statementHandle, char* statementText, int textLength) {
    (void)statementHandle;
    (void)statementText;
    (void)textLength;
    return 0; // Always success
}

int mock_SQLExecute(void* statementHandle) {
    (void)statementHandle;
    // Reset fetch counter on new execution
    mock_fetch_current_row = 0;
    return mock_SQLExecute_result;
}

int mock_SQLFreeStmt(void* statementHandle, int option) {
    (void)statementHandle;
    (void)option;
    return 0; // Always success
}

int mock_SQLDescribeCol(void* statementHandle, int columnNumber, unsigned char* columnName,
                       int bufferLength, short* nameLength, int* dataType, int* columnSize,
                       short* decimalDigits, short* nullable) {
    (void)statementHandle;
    (void)columnNumber;
    (void)dataType;
    (void)columnSize;
    (void)decimalDigits;
    (void)nullable;
    
    if (mock_SQLDescribeCol_result != 0) {
        return mock_SQLDescribeCol_result;
    }
    
    if (columnName && bufferLength > 0) {
        size_t len = (size_t)(bufferLength - 1);
        strncpy((char*)columnName, mock_SQLDescribeCol_column_name, len);
        ((char*)columnName)[len] = '\0';
    }
    
    if (nameLength) {
        *nameLength = (short)strlen(mock_SQLDescribeCol_column_name);
    }
    
    return 0; // SQL_SUCCESS
}

int mock_SQLGetDiagRec(short handleType, void* handle, short recNumber, unsigned char* sqlState,
                      long* nativeError, unsigned char* messageText, short bufferLength, short* textLength) {
    (void)handleType;
    (void)handle;
    (void)recNumber;
    
    if (mock_SQLGetDiagRec_result != 0) {
        return mock_SQLGetDiagRec_result;
    }
    
    if (sqlState) {
        strncpy((char*)sqlState, mock_SQLGetDiagRec_sqlstate, 5);
        ((char*)sqlState)[5] = '\0';
    }
    
    if (nativeError) {
        *nativeError = mock_SQLGetDiagRec_native_error;
    }
    
    if (messageText && bufferLength > 0) {
        size_t len = (size_t)(bufferLength - 1);
        strncpy((char*)messageText, mock_SQLGetDiagRec_message, len);
        ((char*)messageText)[len] = '\0';
    }
    
    if (textLength) {
        *textLength = (short)strlen(mock_SQLGetDiagRec_message);
    }
    
    return 0; // SQL_SUCCESS
}

// Mock control functions
void mock_libdb2_set_SQLAllocHandle_result(int result) {
    mock_SQLAllocHandle_result = result;
}

void mock_libdb2_set_SQLAllocHandle_output_handle(void* handle) {
    mock_SQLAllocHandle_output_handle = handle;
}

void mock_libdb2_set_SQLDriverConnect_result(int result) {
    mock_SQLDriverConnect_result = result;
}

void mock_libdb2_set_SQLExecDirect_result(int result) {
    mock_SQLExecDirect_result = result;
}

void mock_libdb2_set_SQLFreeHandle_result(int result) {
    mock_SQLFreeHandle_result = result;
}

void mock_libdb2_set_SQLExecute_result(int result) {
    mock_SQLExecute_result = result;
}

void mock_libdb2_set_SQLFetch_result(int result) {
    mock_SQLFetch_result = result;
}

void mock_libdb2_set_fetch_row_count(int count) {
    mock_fetch_row_count = count;
    mock_fetch_current_row = 0;
}

void mock_libdb2_set_SQLNumResultCols_result(int result, int column_count) {
    mock_SQLNumResultCols_result = result;
    mock_SQLNumResultCols_column_count = column_count;
}

void mock_libdb2_set_SQLRowCount_result(int result, int row_count) {
    mock_SQLRowCount_result = result;
    mock_SQLRowCount_row_count = row_count;
}

void mock_libdb2_set_SQLDescribeCol_result(int result) {
    mock_SQLDescribeCol_result = result;
}

void mock_libdb2_set_SQLDescribeCol_column_name(const char* name) {
    if (name) {
        strncpy(mock_SQLDescribeCol_column_name, name, sizeof(mock_SQLDescribeCol_column_name) - 1);
        mock_SQLDescribeCol_column_name[sizeof(mock_SQLDescribeCol_column_name) - 1] = '\0';
    }
}

void mock_libdb2_set_SQLGetData_result(int result) {
    mock_SQLGetData_result = result;
}

void mock_libdb2_set_SQLGetData_data(const char* data, int data_len) {
    if (data) {
        strncpy(mock_SQLGetData_data, data, sizeof(mock_SQLGetData_data) - 1);
        mock_SQLGetData_data[sizeof(mock_SQLGetData_data) - 1] = '\0';
        mock_SQLGetData_data_len = data_len;
    }
}

void mock_libdb2_set_SQLGetDiagRec_result(int result) {
    mock_SQLGetDiagRec_result = result;
}

void mock_libdb2_set_SQLGetDiagRec_error(const char* sqlstate, long native_error, const char* message) {
    if (sqlstate) {
        strncpy(mock_SQLGetDiagRec_sqlstate, sqlstate, 5);
        mock_SQLGetDiagRec_sqlstate[5] = '\0';
    }
    mock_SQLGetDiagRec_native_error = native_error;
    if (message) {
        strncpy(mock_SQLGetDiagRec_message, message, sizeof(mock_SQLGetDiagRec_message) - 1);
        mock_SQLGetDiagRec_message[sizeof(mock_SQLGetDiagRec_message) - 1] = '\0';
    }
}

void mock_libdb2_set_SQLEndTran_result(int result) {
    mock_SQLEndTran_result = result;
}

void mock_libdb2_reset_all(void) {
    mock_SQLAllocHandle_result = 0;
    mock_SQLAllocHandle_output_handle = (void*)0x12345678;
    mock_SQLDriverConnect_result = 0;
    mock_SQLExecDirect_result = 0;
    mock_SQLExecute_result = 0;
    mock_SQLFetch_result = 0;
    mock_fetch_row_count = 0;
    mock_fetch_current_row = 0;
    mock_SQLNumResultCols_result = 0;
    mock_SQLNumResultCols_column_count = 1;
    mock_SQLRowCount_result = 0;
    mock_SQLRowCount_row_count = 1;
    mock_SQLDescribeCol_result = 0;
    strncpy(mock_SQLDescribeCol_column_name, "test_column", sizeof(mock_SQLDescribeCol_column_name) - 1);
    mock_SQLGetData_result = 0;
    strncpy(mock_SQLGetData_data, "test_data", sizeof(mock_SQLGetData_data) - 1);
    mock_SQLGetData_data_len = 9;
    mock_SQLGetDiagRec_result = 0;
    memcpy(mock_SQLGetDiagRec_sqlstate, "42000\0", 6);
    mock_SQLGetDiagRec_native_error = 12345;
    strncpy(mock_SQLGetDiagRec_message, "Mock error message", sizeof(mock_SQLGetDiagRec_message) - 1);
    mock_SQLFreeHandle_result = 0;
    mock_SQLEndTran_result = 0;
}