/*
 * Scripting Subsystem - Scoreboard Terminal Pruning
 *
 * Split from scoreboard.c to stay under the 1000-line source limit.
 * Owns the terminal-entry retention helpers declared in scoreboard.h
 * (scoreboard_terminal_is_expired, scoreboard_compact_after_prune,
 * scoreboard_drop_oldest_extra_terminals, scoreboard_prune_terminal*).
 */

#include <src/hydrogen.h>

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "scoreboard.h"

bool scoreboard_terminal_is_expired(const ScoreboardEntry* entry, const struct timespec* now, int min_age_seconds) {
    time_t finished;
    time_t age;

    if (!entry || !now) {
        return false;
    }
    if (!is_terminal_status(entry->status)) {
        return false;
    }
    if (entry->has_waiter) {
        return false;
    }
    if (min_age_seconds <= 0) {
        return true;
    }
    finished = entry->finished_at.tv_sec;
    if (finished <= 0) {
        return false;
    }
    age = now->tv_sec - finished;
    return age >= (time_t)min_age_seconds;
}

void scoreboard_compact_after_prune(Scoreboard* sb, size_t write_idx) {
    size_t i;
    if (!sb) {
        return;
    }
    for (i = write_idx; i < sb->count; i++) {
        memset(&sb->entries[i], 0, sizeof(ScoreboardEntry));
    }
    sb->count = write_idx;
}

size_t scoreboard_drop_oldest_extra_terminals(Scoreboard* sb, size_t pruned) {
    size_t i;
    if (!sb) {
        return pruned;
    }
    while (true) {
        size_t oldest_idx = 0;
        time_t oldest_finished = 0;
        bool found_oldest = false;
        size_t terminal_kept = 0;

        for (i = 0; i < sb->count; i++) {
            if (is_terminal_status(sb->entries[i].status)
                && !sb->entries[i].has_waiter) {
                terminal_kept++;
                if (!found_oldest
                    || sb->entries[i].finished_at.tv_sec < oldest_finished) {
                    oldest_idx = i;
                    oldest_finished = sb->entries[i].finished_at.tv_sec;
                    found_oldest = true;
                }
            }
        }
        if (terminal_kept <= SCOREBOARD_TERMINAL_MAX_RETAINED || !found_oldest) {
            break;
        }
        entry_clear_owned(&sb->entries[oldest_idx]);
        for (i = oldest_idx; i + 1 < sb->count; i++) {
            sb->entries[i] = sb->entries[i + 1];
        }
        sb->count--;
        memset(&sb->entries[sb->count], 0, sizeof(ScoreboardEntry));
        pruned++;
    }
    return pruned;
}

size_t scoreboard_prune_terminal_older_than(Scoreboard* sb, int min_age_seconds) {
    struct timespec now;
    size_t pruned;
    size_t write_idx;
    size_t i;
    if (!sb) {
        return 0;
    }
    pthread_mutex_lock(&sb->mutex);

    timespec_now(&now);
    pruned = 0;
    write_idx = 0;
    for (i = 0; i < sb->count; i++) {
        if (scoreboard_terminal_is_expired(&sb->entries[i], &now, min_age_seconds)) {
            entry_clear_owned(&sb->entries[i]);
            pruned++;
        } else {
            if (write_idx != i) {
                sb->entries[write_idx] = sb->entries[i];
            }
            write_idx++;
        }
    }
    scoreboard_compact_after_prune(sb, write_idx);
    pruned = scoreboard_drop_oldest_extra_terminals(sb, pruned);

    pthread_mutex_unlock(&sb->mutex);
    return pruned;
}

size_t scoreboard_prune_terminal(Scoreboard* sb) {
    return scoreboard_prune_terminal_older_than(sb, SCOREBOARD_TERMINAL_RETENTION_SECONDS);
}

