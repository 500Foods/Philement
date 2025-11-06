/*
 * Database Queue Lead - Migration LOAD Phase
 *
 * LOAD Phase: Extract migration metadata from Lua scripts and populate Queries table
 * - Execute Lua migration scripts to generate SQL templates
 * - INSERT records into Queries table with type = 1000 (loaded status)
 * - NO database schema changes occur in this phase
 * - Only metadata population for later APPLY phase execution
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/database_bootstrap.h>
#include <src/database/migration/migration.h>

// Local includes
#include "dbqueue.h"

/*
 * Execute migration load phase
 *
 * LOAD Phase: Extract migration metadata from Lua scripts and populate Queries table
 * - Execute Lua migration scripts to generate SQL templates
 * - INSERT records into Queries table with type = 1000 (loaded status)
 * - NO database schema changes occur in this phase
 * - Only metadata population for later APPLY phase execution
 */
bool database_queue_lead_execute_migration_load(DatabaseQueue* lead_queue) {
    char* dqm_label = database_queue_generate_label(lead_queue);
    log_this(dqm_label, "Starting migration LOAD phase - populating Queries table metadata", LOG_LEVEL_DEBUG, 0);

    // Execute LOAD migrations (populate Queries table metadata only) - LOAD PHASE
    bool load_success = execute_load_migrations(lead_queue, lead_queue->persistent_connection);

    if (load_success) {
        log_this(dqm_label, "Migration LOAD phase completed successfully - Queries table populated with migration metadata", LOG_LEVEL_DEBUG, 0);
    } else {
        log_this(dqm_label, "Migration LOAD phase failed - could not populate Queries table", LOG_LEVEL_ERROR, 0);
    }

    free(dqm_label);
    return load_success;
}