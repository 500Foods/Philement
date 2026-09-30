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
short mssql_mock_SQLAllocHandle(short handleType, void* inputHandle, void** outputHandle);
short mssql_mock_SQLConnect(void* connectionHandle, char* serverName, int nameLength,
                   char* userName, int userLength, char* password, int passwordLength);
short mssql_mock_SQLDriverConnect(void* connectionHandle, void* windowHandle, unsigned char* connectionString,
                           short stringLength, unsigned char* outConnectionString, short bufferLength,
                           short* stringLengthPtr, unsigned short driverCompletion);
short mssql_mock_SQLExecDirect(void* statementHandle, char* statementText, int textLength);
short mssql_mock_SQLFetch(void* statementHandle);
short mssql_mock_SQLGetData(void* statementHandle, int columnNumber, int targetType,
                     void* targetValue, long bufferLength, long* strLenOrIndPtr);
short mssql_mock_SQLNumResultCols(void* statementHandle, int* columnCount);
short mssql_mock_SQLRowCount(void* statementHandle, long* rowCount);
short mssql_mock_SQLFreeHandle(short handleType, void* handle);
short mssql_mock_SQLDisconnect(void* connectionHandle);
short mssql_mock_SQLEndTran(short handleType, void* handle, int completionType);
short mssql_mock_SQLPrepare(void* statementHandle, unsigned char* statementText, int textLength);
short mssql_mock_SQLExecute(void* statementHandle);
short mssql_mock_SQLFreeStmt(void* statementHandle, int option);
short mssql_mock_SQLDescribeCol(void* statementHandle, int columnNumber, unsigned char* columnName,
                         int bufferLength, short* nameLength, int* dataType, int* columnSize,
                         short* decimalDigits, short* nullable);
short mssql_mock_SQLGetDiagRec(short handleType, void* handle, short recNumber, unsigned char* sqlState,
                       long* nativeError, unsigned char* messageText, short bufferLength, short* textLength);
short mssql_mock_SQLSetConnectAttr(void* connectionHandle, int attribute, const void* value, int stringLength);
short mssql_mock_SQLBindParameter(void* statementHandle, unsigned short parameterNumber, short inputOutputType,
                           short valueType, short parameterType, unsigned long columnSize, short decimalDigits,
                           void* parameterValue, long bufferLength, long* strLenOrIndPtr);
short mssql_mock_SQLCancel(void* statementHandle);
short mssql_mock_SQLSetEnvAttr(void* environmentHandle, int attribute, void* value, int stringLength);

// Mock control functions for tests (always available)
void mssql_mock_libodbc_set_SQLAllocHandle_result(short result);
void mssql_mock_libodbc_set_SQLAllocHandle_output_handle(void* handle);
void mssql_mock_libodbc_set_SQLDriverConnect_result(short result);
void mssql_mock_libodbc_set_SQLExecDirect_result(short result);
void mssql_mock_libodbc_set_SQLExecute_result(short result);
void mssql_mock_libodbc_set_SQLFetch_result(short result);
void mssql_mock_libodbc_set_SQLNumResultCols_result(short result, int column_count);
void mssql_mock_libodbc_set_SQLRowCount_result(short result, int row_count);
void mssql_mock_libodbc_set_SQLDescribeCol_result(short result);
void mssql_mock_libodbc_set_SQLDescribeCol_column_name(const char* name);
void mssql_mock_libodbc_set_SQLGetData_result(short result);
void mssql_mock_libodbc_set_SQLGetData_data(const char* data, int data_len);
void mssql_mock_libodbc_set_SQLGetDiagRec_result(short result);
void mssql_mock_libodbc_set_SQLGetDiagRec_error(const char* sqlstate, long native_error, const char* message);
void mssql_mock_libodbc_set_fetch_row_count(int count);
void mssql_mock_libodbc_set_SQLFreeHandle_result(short result);
void mssql_mock_libodbc_set_SQLEndTran_result(short result);
void mssql_mock_libodbc_set_SQLSetConnectAttr_result(short result);
int mssql_mock_libodbc_get_SQLSetConnectAttr_autocommit_off_seen(void);
int mssql_mock_libodbc_get_SQLSetConnectAttr_call_count(void);
void mssql_mock_libodbc_set_SQLPrepare_result(short result);
void mssql_mock_libodbc_set_SQLBindParameter_result(short result);
void mssql_mock_libodbc_reset_all(void);

#endif // MOCK_LIBODBC_H
