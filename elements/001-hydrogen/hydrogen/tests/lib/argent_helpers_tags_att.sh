#!/usr/bin/env bash
# shellcheck disable=SC2154 # exercise-local variables (token, etc.) are set by argent_mcp_exercise

# Argent tags, attachments, and transaction-get exercise for tests/test_73_argent_mcp.sh.
# Split out of argent_mcp_helpers.sh to keep both files under the 1,000-line cap.
#
# Depends on: argent_mcp_helpers.sh (argent_ok, argent_fail, argent_miss,
# argent_grab) and the exercise-local variables from argent_mcp_exercise:
# token, bank_id, card_id, org_id, txn_id, line_id, att_id

# CHANGELOG
# 1.0.0 - 2026-10-09 - Split from argent_mcp_helpers.sh for code-size cap

argent_exercise_tags_att() {
    local args

    argent_fail tags_type Argent.AddTags '{"entity_type":0,"entity_id":1,"tags":[{"name":"x"}]}' entity_type || true
    args=$(jq -n --argjson id "${bank_id:-0}" '{entity_type:2,entity_id:$id,tags:[]}')
    argent_fail tags_empty Argent.AddTags "${args}" tags_required "${bank_id}" || true
    args=$(jq -n --argjson id "${bank_id:-0}" '{entity_type:2,entity_id:$id,tags:[1]}')
    argent_fail tags_item Argent.AddTags "${args}" tags "${bank_id}" || true
    args=$(jq -n --argjson id "${bank_id:-0}" '{entity_type:2,entity_id:$id,tags:[{}]}')
    argent_fail tags_name Argent.AddTags "${args}" name_required "${bank_id}" || true
    args=$(jq -n --argjson id "${bank_id:-0}" --arg name "tag${token}" \
        '{entity_type:2,entity_id:$id,tags:[{name:$name}]}')
    argent_ok tags_create Argent.AddTags "${args}" \
        '(.result.structuredContent.links|length) == 1 and .result.structuredContent.links[0].created == true' \
        "${bank_id}" || true
    tag_id=$(argent_grab tags_create '.result.structuredContent.links[0].tag_id | tonumber')
    args=$(jq -n --argjson id "${bank_id:-0}" --argjson tag "${tag_id:-0}" \
        '{entity_type:2,entity_id:$id,tags:[{tag_id:$tag}]}')
    argent_ok tags_again Argent.AddTags "${args}" \
        '.result.structuredContent.links[0].created == false' \
        "${bank_id}" "${tag_id}" || true
    args=$(jq -n --argjson id "${bank_id:-0}" '{entity_type:2,entity_id:$id,tags:[{tag_id:-1}]}')
    argent_fail tags_unknown Argent.AddTags "${args}" not_found "${bank_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --arg name "tag${token}" '{organization_id:$org,tag:$name}')
    if [[ "${bank_id}" =~ ^[0-9]+$ ]]; then
        argent_ok tags_list_hit Argent.ListLedgers "${args}" \
            "([.result.structuredContent.ledgers[]?.ledger_id | tonumber] | index(${bank_id})) != null" \
            "${org_id}" || true
    else
        argent_miss tags_list_hit
    fi
    args=$(jq -n --argjson id "${bank_id:-0}" --argjson tag "${tag_id:-0}" \
        '{entity_type:2,entity_id:$id,tag_id:$tag}')
    argent_ok tags_remove Argent.RemoveTags "${args}" \
        '(.result.structuredContent.removed[0].removed == true)' \
        "${bank_id}" "${tag_id}" || true
    args=$(jq -n --argjson org "${org_id:-0}" --arg name "tag${token}" '{organization_id:$org,tag:$name}')
    if [[ "${bank_id}" =~ ^[0-9]+$ ]]; then
        argent_ok tags_list_miss Argent.ListLedgers "${args}" \
            "([.result.structuredContent.ledgers[]?.ledger_id | tonumber] | index(${bank_id})) == null" \
            "${org_id}" || true
    else
        argent_miss tags_list_miss
    fi
    args=$(jq -n --argjson id "${bank_id:-0}" '{entity_type:2,entity_id:$id,tag_id:-1}')
    argent_ok tags_remove_absent Argent.RemoveTags "${args}" \
        '.result.structuredContent.removed[0].removed == false' \
        "${bank_id}" || true
    argent_fail tags_remove_required Argent.RemoveTags '{"entity_type":2,"entity_id":1}' tag_id_required || true
    args=$(jq -n --argjson id "${bank_id:-0}" '{entity_type:2,entity_id:$id,tag_ids:["x"]}')
    argent_fail tags_ids_bad Argent.RemoveTags "${args}" tag_id "${bank_id}" || true
    args=$(jq -n --argjson id "${txn_id:-0}" --arg name "txntag${token}" \
        '{entity_type:3,entity_id:$id,tags:[{name:$name}]}')
    argent_ok tags_txn Argent.AddTags "${args}" '.result.structuredContent.links[0].created == true' \
        "${txn_id}" || true
    args=$(jq -n --argjson id "${line_id:-0}" --arg name "linetag${token}" \
        '{entity_type:4,entity_id:$id,tags:[{name:$name}]}')
    argent_ok tags_line Argent.AddTags "${args}" '.result.structuredContent.links[0].created == true' \
        "${line_id}" || true
    args=$(jq -n --argjson id "${txn_id:-0}" '{txn_id:$id}')
    if [[ "${txn_id}" =~ ^[0-9]+$ ]]; then
        argent_ok txn_get_tags Argent.GetTransaction "${args}" \
            "([.result.structuredContent.tags[]?.name] | index(\"txntag${token}\")) != null and ([.result.structuredContent.tags[]?.name] | index(\"linetag${token}\")) == null" \
            "${txn_id}" || true
    else
        argent_miss txn_get_tags
    fi

    args=$(jq -n --argjson id "${txn_id:-0}" '{entity_type:3,entity_id:$id,name:"note",att_type:9}')
    argent_fail att_type Argent.AddAttachment "${args}" att_type "${txn_id}" || true
    args=$(jq -n --argjson id "${txn_id:-0}" '{entity_type:3,entity_id:$id,name:"note",file_text:"hello",byte_len:-1}')
    argent_fail att_byte Argent.AddAttachment "${args}" byte_len "${txn_id}" || true
    args=$(jq -n --argjson id "${txn_id:-0}" --arg text "hello note" \
        '{entity_type:3,entity_id:$id,name:"Note",file_text:$text}')
    argent_ok att_note Argent.AddAttachment "${args}" \
        '.result.structuredContent.created == true and (.result.structuredContent.rev_id | tonumber) == 1' \
        "${txn_id}" || true
    att_id=$(argent_grab att_note '.result.structuredContent.attachment_id | tonumber')
    args=$(jq -n --argjson entity "${txn_id:-0}" --argjson id "${att_id:-0}" --arg text "hello note 2" \
        '{attachment_id:$id,entity_type:3,entity_id:$entity,name:"Note",file_text:$text}')
    argent_ok att_note_rev Argent.AddAttachment "${args}" \
        '(.result.structuredContent.rev_id | tonumber) == 2 and (.result.structuredContent.attachment_id | tonumber) == '"${att_id:-0}" \
        "${txn_id}" "${att_id}" || true
    args=$(jq -n --argjson id "${txn_id:-0}" '{entity_type:3,entity_id:$id,name:"File",file_data:"abc",file_name:"a.txt"}')
    argent_ok att_file Argent.AddAttachment "${args}" \
        '.result.structuredContent.created == true and (.result.structuredContent.rev_id | tonumber) == 1' \
        "${txn_id}" || true
    args=$(jq -n --argjson id "${txn_id:-0}" '{txn_id:$id}')
    if [[ "${att_id}" =~ ^[0-9]+$ ]]; then
        argent_ok txn_get_note Argent.GetTransaction "${args}" \
            "(( [.result.structuredContent.attachments[] | select(((.attachment_id|tonumber)==${att_id}) and ((.rev_id|tonumber)==1)) | .att_type_a2010 | tonumber] | .[0] ) == 1) and (( [.result.structuredContent.attachments[] | select(((.attachment_id|tonumber)==${att_id}) and ((.rev_id|tonumber)==1)) | .mime_type] | .[0] ) == \"text/plain\")" \
            "${txn_id}" || true
        argent_ok txn_get_file Argent.GetTransaction "${args}" \
            '([.result.structuredContent.attachments[] | select(.name == "File") | .att_type_a2010 | tonumber] | .[0]) == 5 and ([.result.structuredContent.attachments[] | has("file_data") or has("file_text")] | any | not)' \
            "${txn_id}" || true
    else
        argent_miss txn_get_note
        argent_miss txn_get_file
    fi
}
