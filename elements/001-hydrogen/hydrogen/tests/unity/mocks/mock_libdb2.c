/*
 * Mock libdb2 functions for unit testing
 *
 * This file provides mock implementations of libdb2 functions
 * to enable testing of DB2 database operations.
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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
    return mock_SQLExecDirect_result;
}

int mock_SQLFetch(void* statementHandle) {
    (void)statementHandle;
    return 0; // Always success
}

int mock_SQLGetData(void* statementHandle, int columnNumber, int targetType,
                   void* targetValue, int bufferLength, int* strLenOrIndPtr) {
    (void)statementHandle;
    (void)columnNumber;
    (void)targetType;
    (void)targetValue;
    (void)bufferLength;
    (void)strLenOrIndPtr;
    return 0; // Always success
}

int mock_SQLNumResultCols(void* statementHandle, int* columnCount) {
    (void)statementHandle;
    if (columnCount) *columnCount = 1;
    return 0; // Always success
}

int mock_SQLRowCount(void* statementHandle, int* rowCount) {
    (void)statementHandle;
    if (rowCount) *rowCount = 1;
    return 0; // Always success
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
    return 0; // Always success
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
    (void)columnName;
    (void)bufferLength;
    (void)nameLength;
    (void)dataType;
    (void)columnSize;
    (void)decimalDigits;
    (void)nullable;
    return 0; // Always success
}

int mock_SQLGetDiagRec(short handleType, void* handle, short recNumber, unsigned char* sqlState,
                      long* nativeError, unsigned char* messageText, short bufferLength, short* textLength) {
    (void)handleType;
    (void)handle;
    (void)recNumber;
    (void)sqlState;
    (void)nativeError;
    (void)messageText;
    (void)bufferLength;
    (void)textLength;
    return 0; // Always success
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

void mock_libdb2_set_SQLEndTran_result(int result) {
    mock_SQLEndTran_result = result;
}

void mock_libdb2_reset_all(void) {
    mock_SQLAllocHandle_result = 0;
    mock_SQLAllocHandle_output_handle = (void*)0x12345678;
    mock_SQLDriverConnect_result = 0;
    mock_SQLExecDirect_result = 0;
    mock_SQLFreeHandle_result = 0;
    mock_SQLEndTran_result = 0;
}