#!/usr/bin/env bash

# render_check.sh
# Compares Reports.Render in the working tree with Reports.Render at a git
# revision, on every reporting fixture (compose and html), byte for byte.
# Usage: ./render_check.sh [rev] [empty|error]
#   rev     git revision to compare against (default HEAD).
#           8b524425f^ is the last single-body Reports.Render.
#   mode    lookup 078 reply in the harness: empty rows (default) or an error.
# Each side replays the acuranzo migrations that set cfg.SCRIPT_NAME to
# "Render" or "Render.<Module>", in file order, so the newest body of each row
# wins. Needs lua 5.4+ with the brotli module that Helium uses, and git.

# CHANGELOG
# 1.0.0 - 2026-11-09 - Initial version (Reports.Render split tooling)

set -euo pipefail

rev="${1:-HEAD}"
mode="${2:-empty}"
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(git -C "${here}" rev-parse --show-toplevel)"
mig="elements/002-helium/acuranzo/migrations"

work="$(mktemp -d)"
trap 'rm -rf "${work}"' EXIT
mkdir -p "${work}/old_tree" "${work}/old" "${work}/new"

git -C "${root}" archive "${rev}" "${mig}" | tar -x -C "${work}/old_tree"

capture_all() {
    local from="$1" to="$2" file
    local -a files=()
    for file in "${from}"/acuranzo_*.lua; do
        if grep -qE 'cfg\.SCRIPT_NAME *= *"Render(\.[A-Za-z]+)?"' "${file}"; then
            files+=("${file}")
        fi
    done
    for file in "${files[@]}"; do
        lua "${here}/capture.lua" "${file}" "${to}" > /dev/null
    done
}

capture_all "${work}/old_tree/${mig}" "${work}/old"
capture_all "${root}/${mig}" "${work}/new"

# Mirror production scripts.options.sandbox.io (1511 on Reports.Render).
# Metrics needs io; only enable when that module is in the capture.
write_options() {
    local dir="$1"
    if [[ -f "${dir}/Metrics.lua" ]]; then
        printf '%s\n' '{"sandbox":{"io":true}}' > "${dir}/options.json"
    else
        printf '%s\n' '{}' > "${dir}/options.json"
    fi
}
write_options "${work}/old"
write_options "${work}/new"

old_list=("${work}"/old/*.lua)
new_list=("${work}"/new/*.lua)
echo "old (${rev}): ${old_list[*]##*/}"
echo "new (working tree): ${new_list[*]##*/}"
lua "${here}/harness.lua" "${work}/old" "${work}/new" "${mode}"
