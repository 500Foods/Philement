/*
 * Mock libdb2 functions for unit testing
 *
 * This file provides mock implementations of libdb2 functions
 * to enable testing of DB2 database operations.
 */

#ifndef MOCK_LIBDB2_H
#define MOCK_LIBDB2_H

#include <stddef.h>
#include <stdbool.h>

// Mock function prototypes (always available) - must match types.h exactly
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
int mock_SQLPrepare(void* statementHandle, unsigned char* statementText, int textLength);
int mock_SQLExecute(void* statementHandle);
int mock_SQLFreeStmt(void* statementHandle, int option);
int mock_SQLDescribeCol(void* statementHandle, int columnNumber, unsigned char* columnName,
                       int bufferLength, short* nameLength, int* dataType, int* columnSize,
                       short* decimalDigits, short* nullable);
int mock_SQLGetDiagRec(short handleType, void* handle, short recNumber, unsigned char* sqlState,
                       long* nativeError, unsigned char* messageText, short bufferLength, short* textLength);
int mock_SQLSetConnectAttr(void* connectionHandle, int attribute, long value, int stringLength);

// Mock control functions for tests (always available)
void mock_libdb2_set_SQLAllocHandle_result(int result);
void mock_libdb2_set_SQLAllocHandle_output_handle(void* handle);
void mock_libdb2_set_SQLDriverConnect_result(int result);
void mock_libdb2_set_SQLExecDirect_result(int result);
void mock_libdb2_set_SQLExecute_result(int result);
void mock_libdb2_set_SQLFetch_result(int result);
void mock_libdb2_set_SQLNumResultCols_result(int result, int column_count);
void mock_libdb2_set_SQLRowCount_result(int result, int row_count);
void mock_libdb2_set_SQLDescribeCol_result(int result);
void mock_libdb2_set_SQLDescribeCol_column_name(const char* name);
void mock_libdb2_set_SQLGetData_result(int result);
void mock_libdb2_set_SQLGetData_data(const char* data, int data_len);
void mock_libdb2_set_SQLGetDiagRec_result(int result);
void mock_libdb2_set_SQLGetDiagRec_error(const char* sqlstate, long native_error, const char* message);
void mock_libdb2_set_fetch_row_count(int count);
void mock_libdb2_set_SQLFreeHandle_result(int result);
void mock_libdb2_set_SQLEndTran_result(int result);
void mock_libdb2_set_SQLSetConnectAttr_result(int result);
void mock_libdb2_set_SQLPrepare_result(int result);
void mock_libdb2_reset_all(void);

#endif // MOCK_LIBDB2_H