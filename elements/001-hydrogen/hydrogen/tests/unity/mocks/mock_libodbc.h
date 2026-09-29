/*
 * Mock libodbc functions for unit testing
 *
 * This file provides mock implementations of libodbc (unixODBC) functions
 * to enable testing of MSSQL database operations.
 * It mirrors mock_libdb2 but is mssql-owned so DB2 tests do not collide.
 * All function names are prefixed with mssql_mock_ to avoid symbol collision.
 */

#ifndef MOCK_LIBODBC_H
#define MOCK_LIBODBC_H

#include <stddef.h>
#include <stdbool.h>

// Mock function prototypes (always available) - must match types.h exactly
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
int mssql_mock_SQLSetConnectAttr(void* connectionHandle, int attribute, long value, int stringLength);
int mssql_mock_SQLBindParameter(void* statementHandle, unsigned short parameterNumber, short inputOutputType,
                           short valueType, short parameterType, unsigned long columnSize, short decimalDigits,
                           void* parameterValue, long bufferLength, long* strLenOrIndPtr);
int mssql_mock_SQLCancel(void* statementHandle);

// Mock control functions for tests (always available)
void mssql_mock_libodbc_set_SQLAllocHandle_result(int result);
void mssql_mock_libodbc_set_SQLAllocHandle_output_handle(void* handle);
void mssql_mock_libodbc_set_SQLDriverConnect_result(int result);
void mssql_mock_libodbc_set_SQLExecDirect_result(int result);
void mssql_mock_libodbc_set_SQLExecute_result(int result);
void mssql_mock_libodbc_set_SQLFetch_result(int result);
void mssql_mock_libodbc_set_SQLNumResultCols_result(int result, int column_count);
void mssql_mock_libodbc_set_SQLRowCount_result(int result, int row_count);
void mssql_mock_libodbc_set_SQLDescribeCol_result(int result);
void mssql_mock_libodbc_set_SQLDescribeCol_column_name(const char* name);
void mssql_mock_libodbc_set_SQLGetData_result(int result);
void mssql_mock_libodbc_set_SQLGetData_data(const char* data, int data_len);
void mssql_mock_libodbc_set_SQLGetDiagRec_result(int result);
void mssql_mock_libodbc_set_SQLGetDiagRec_error(const char* sqlstate, long native_error, const char* message);
void mssql_mock_libodbc_set_fetch_row_count(int count);
void mssql_mock_libodbc_set_SQLFreeHandle_result(int result);
void mssql_mock_libodbc_set_SQLEndTran_result(int result);
void mssql_mock_libodbc_set_SQLSetConnectAttr_result(int result);
int mssql_mock_libodbc_get_SQLSetConnectAttr_autocommit_off_seen(void);
int mssql_mock_libodbc_get_SQLSetConnectAttr_call_count(void);
void mssql_mock_libodbc_set_SQLPrepare_result(int result);
void mssql_mock_libodbc_set_SQLBindParameter_result(int result);
void mssql_mock_libodbc_reset_all(void);

#endif // MOCK_LIBODBC_H
