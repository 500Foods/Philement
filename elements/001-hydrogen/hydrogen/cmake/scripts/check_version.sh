#!/bin/bash
# check_version.sh - Script to check if the version number needs updating based on git commit count

# Check if the correct number of arguments are provided
if [ $# -ne 2 ]; then
    echo "❌ Error: Incorrect number of arguments."
    echo "Usage: $0 <project_root> <current_commit_count>"
    exit 1
fi

# shellcheck disable=SC2034
PROJECT_ROOT="$1"  # Kept for potential future use or reconfiguration command
CURRENT_COMMIT_COUNT="$2"

# Get the latest git commit count
LATEST_COMMIT_COUNT=$(git log --oneline | wc -l)

# Check if we are in a git repository
if ! git log --oneline >/dev/null 2>&1; then
    echo "⚠️ Warning: Not in a git repository or git not installed. Cannot check commit count."
    echo "Using provided commit count: $CURRENT_COMMIT_COUNT"
    exit 0
fi

# Compare the commit counts
if [ "$LATEST_COMMIT_COUNT" -ne "$CURRENT_COMMIT_COUNT" ]; then
    echo "⚠️ Warning: Git commit count has changed!"
    echo "  Current build commit count: $CURRENT_COMMIT_COUNT"
    echo "  Latest commit count: $LATEST_COMMIT_COUNT"
    echo "  The version number may be outdated. Consider re-running the configuration step with:"
    echo "    cmake -S cmake -B build"
    echo "  This will update the version number based on the latest commit count."
    # Optionally, you can force a reconfiguration here if desired
    # cd "$PROJECT_ROOT" && cmake -S cmake -B build
else
    echo "✅ Version number is up-to-date with the latest git commit count: $CURRENT_COMMIT_COUNT"
fi

exit 0
