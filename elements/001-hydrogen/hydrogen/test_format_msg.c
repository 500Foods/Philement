#include <stdio.h>
#include <src/api/conduit/helpers/param_proc_helpers.h>

int main() {
    char* msg1 = format_type_error_message("userId", "string", "INTEGER", "is not");
    printf("msg1: %s\n", msg1);
    free(msg1);
    
    char* msg2 = format_type_error_message("email", "integer", "STRING", "should be");
    printf("msg2: %s\n", msg2);
    free(msg2);
    
    char* msg3 = format_type_error_message("user_name", "boolean", "FLOAT", "is not");
    printf("msg3: %s\n", msg3);
    free(msg3);
    
    return 0;
}
