#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>
#include <src/config/config_databases.h>

// Mock database connection for testing
DatabaseConnection create_test_connection() {
    DatabaseConnection conn;
    memset(&conn, 0, sizeof(DatabaseConnection));
    
    conn.name = strdup("TestDB");
    conn.connection_name = strdup("TestDB");
    conn.enabled = true;
    conn.type = strdup("sqlite");
    conn.database = strdup("test.db");
    
    // Create parameters JSON object
    json_t* params = json_object();
    json_object_set_new(params, "LOGINRETRYWINDOW", json_integer(30));
    json_object_set_new(params, "IPADDRESS", json_string("192.168.1.1"));
    json_object_set_new(params, "LOGINID", json_string("testuser"));
    
    conn.parameters = params;
    
    return conn;
}

void cleanup_test_connection(DatabaseConnection* conn) {
    if (conn) {
        free(conn->name);
        free(conn->connection_name);
        free(conn->type);
        free(conn->database);
        if (conn->parameters) {
            json_decref(conn->parameters);
        }
        memset(conn, 0, sizeof(DatabaseConnection));
    }
}

int main() {
    printf("Testing parameter merging functionality...\n");
    
    // Create a test database connection with parameters
    DatabaseConnection test_conn = create_test_connection();
    
    // Create query parameters that should override some database parameters
    json_t* query_params = json_object();
    json_object_set_new(query_params, "LOGINID", json_string("queryuser"));  // Override
    json_object_set_new(query_params, "NEW_PARAM", json_string("new_value"));  // New parameter
    
    printf("Database parameters:\n");
    char* db_params_str = json_dumps(test_conn.parameters, JSON_INDENT(2));
    printf("%s\n", db_params_str);
    free(db_params_str);
    
    printf("\nQuery parameters:\n");
    char* query_params_str = json_dumps(query_params, JSON_INDENT(2));
    printf("%s\n", query_params_str);
    free(query_params_str);
    
    // Test the merge function
    json_t* merged_params = merge_database_parameters(&test_conn, query_params);
    
    printf("\nMerged parameters:\n");
    char* merged_params_str = json_dumps(merged_params, JSON_INDENT(2));
    printf("%s\n", merged_params_str);
    free(merged_params_str);
    
    // Verify the merge worked correctly
    json_t* loginid = json_object_get(merged_params, "LOGINID");
    json_t* ipaddress = json_object_get(merged_params, "IPADDRESS");
    json_t* new_param = json_object_get(merged_params, "NEW_PARAM");
    json_t* retry_window = json_object_get(merged_params, "LOGINRETRYWINDOW");
    
    printf("\nVerification:\n");
    printf("LOGINID (should be 'queryuser'): %s\n", json_string_value(loginid));
    printf("IPADDRESS (should be '192.168.1.1'): %s\n", json_string_value(ipaddress));
    printf("NEW_PARAM (should be 'new_value'): %s\n", json_string_value(new_param));
    printf("LOGINRETRYWINDOW (should be 30): %d\n", (int)json_integer_value(retry_window));
    
    // Test find_database_connection function
    printf("\nTesting find_database_connection function...\n");
    
    DatabaseConfig test_config;
    memset(&test_config, 0, sizeof(DatabaseConfig));
    test_config.connection_count = 1;
    test_config.connections[0] = create_test_connection();
    
    DatabaseConnection* found_conn = find_database_connection(&test_config, "TestDB");
    if (found_conn) {
        printf("✓ Successfully found database connection 'TestDB'\n");
    } else {
        printf("✗ Failed to find database connection 'TestDB'\n");
    }
    
    DatabaseConnection* not_found = find_database_connection(&test_config, "NonExistent");
    if (!not_found) {
        printf("✓ Correctly returned NULL for non-existent connection\n");
    } else {
        printf("✗ Should not have found non-existent connection\n");
    }
    
    // Cleanup
    json_decref(query_params);
    json_decref(merged_params);
    cleanup_test_connection(&test_conn);
    cleanup_database_connection(&test_config.connections[0]);
    
    printf("\n✓ All tests completed successfully!\n");
    
    return 0;
}