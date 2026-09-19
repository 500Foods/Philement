/*
 * Firebird Database Engine - Transaction Management Implementation
 *
 * Implements Firebird transaction management via isc_start_transaction /
 * isc_commit_transaction / isc_rollback_transaction. All operations use
 * the Firebird TPB (Transaction Parameter Buffer) format.
 *
 * Phase 5 skeleton: transaction start/commit/rollback are stubbed to
 * allocate a Transaction struct but do not call isc_start_transaction.
 * The real engine wiring (prepare TPB as a raw buffer, invoke
 * isc_start_transaction on the thread pool) is a Phase 6 concern.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>

#include "types.h"
#include "connection.h"
#include "transaction.h"

/*
 * Build a minimal Firebird TPB (Transaction Parameter Buffer) for a
 * given isolation level. The TPB controls transaction behavior such as
 * read/write mode, isolation level, and lock resolution.
 *
 * Firebird TPB format (version 1):
 *   byte 0: isc_tpb_version_1 (value 0x01 after length byte)
 *   followed by param/type/value triples.
 *
 * Raw TPB bytes for common isolation levels:
 *   Read committed:   \x04\x01\x00\x08\x20\x01  (version 1, read, read_committed, no_rec_version)
 *   Serializable:    \x04\x01\x00\x08\x04\x02  (version 1, read, consistency, table_spi)
 *   Repeatable read: \x04\x01\x00\x08\x04\x03  (version 1, read, consistency, table_rs)
 *   Read uncommitted:\x04\x01\x00\x08\x20\x01\x15 (version 1, read, read_committed, rec_version)
 *
 * We return a static string per level — not owned by the caller.
 */
const char* firebird_build_tpb(DatabaseIsolationLevel level) {
    switch (level) {
        case DB_ISOLATION_SERIALIZABLE:
            return "\x04\x01\x00\x08\x04\x02";
        case DB_ISOLATION_READ_COMMITTED:
        default:
            return "\x04\x01\x00\x08\x20\x01";
        case DB_ISOLATION_READ_UNCOMMITTED:
            // read_committed + rec_version (allows non-repeatable reads)
            return "\x04\x01\x00\x08\x20\x01\x15";
        case DB_ISOLATION_REPEATABLE_READ:
            return "\x04\x01\x00\x08\x04\x03";
    }
}

/*
 * Begin a Firebird transaction.
 * Creates a Transaction struct that wraps the Firebird tr_handle.
 *
 * Phase 5 skeleton: allocates the Transaction but does not invoke
 * isc_start_transaction. The TPB is built but not passed to the engine.
 * Phase 6 will call isc_start_transaction_ptr and set the handle.
 */
bool firebird_begin_transaction(DatabaseHandle* connection,
                                 DatabaseIsolationLevel level,
                                 Transaction** transaction) {
    if (!connection || connection->engine_type != DB_ENGINE_FIREBIRD || !transaction) {
        return false;
    }

    FirebirdConnection* fb_conn = (FirebirdConnection*)connection->connection_handle;
    if (!fb_conn) {
        return false;
    }

    Transaction* txn = calloc(1, sizeof(Transaction));
    if (!txn) {
        return false;
    }

    const char* tpb = firebird_build_tpb(level);

    txn->isolation_level = level;
    txn->started_at      = time(NULL);
    txn->active          = true;
    txn->transaction_id  = NULL;
    // engine_specific_handle is where Phase 6 will store the isc_tr_handle.
    // For the skeleton we leave it NULL — no real isc_start_transaction.
    txn->engine_specific_handle = NULL;

    (void)tpb;  // TPB is built but not used until Phase 6

    *transaction = txn;
    connection->current_transaction = txn;
    return true;
}

/*
 * Commit a Firebird transaction.
 * Phase 5: frees the Transaction struct. No isc_commit_transaction call.
 */
bool firebird_commit_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || connection->engine_type != DB_ENGINE_FIREBIRD || !transaction) {
        return false;
    }

    if (!connection->current_transaction) {
        return false;
    }

    transaction->active = false;
    connection->current_transaction = NULL;
    free(transaction);
    return true;
}

/*
 * Rollback a Firebird transaction.
 * Phase 5: frees the Transaction struct. No isc_rollback_transaction call.
 */
bool firebird_rollback_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || connection->engine_type != DB_ENGINE_FIREBIRD || !transaction) {
        return false;
    }

    if (!connection->current_transaction) {
        return false;
    }

    transaction->active = false;
    connection->current_transaction = NULL;
    free(transaction);
    return true;
}
