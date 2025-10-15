-- database.lua
-- Defines the Helium schema, supported database engines, and SQL defaults for migrations used in Hydrogen

--[[
    CHANGELOG
    2025-09-13 | 1.0.0      | Andrew Simard     | Initial creation with support for SQLite, PostgreSQL, MySQL, DB2
]]

local database = {

    -- Lookup #27 - Query Status
    query_status = {
        inactive = 0,
        active = 1
    },

    -- Lookup #28 - Query types
    query_types = {
        internal_sql = 0,
        system_sql = 1,
        system_ddl = 2,
        reporting_sql = 3,
        forward_migration = 1000,
        reverse_migration = 1001,
        diagram_migration = 1002
    },

    -- Currently supported database engines - mirrors Lookup #30
    engines = {
        postgresql = true,
        sqlite = true,
        mysql = true,
        db2 = true
    },

    -- Lookup #30 - Query Dialects
    query_dialects = {
        postgresql = 1,
        sqlite = 2,
        mysql = 3,
        db2 = 4
    },

    -- Saves repeating it in virtually every single template
    query_insert_columns =  [[
                              query_id,
                              query_ref,
                              query_type_lua_28,
                              query_dialect_lua_30,
                              name,
                              summary,
                              query_code,
                              query_status_lua_27,
                              collection,
                              valid_after,
                              valid_until,
                              created_id,
                              created_at,
                              updated_id,
                              updated_at
                            ]],

    -- This is the default for a great many queries                            
    query_common_boilerplate =  [[
                                  NULL,                                   -- valid_after
                                  NULL,                                   -- valid_until
                                  0,                                      -- created_id
                                  ${NOW},                                 -- created_at
                                  0,                                      -- updated_id
                                  ${NOW}                                  -- updated_at
                                ]],                            

    -- Load engine-specific configurations from separate files
    defaults = {
        sqlite = require("database_sqlite"),
        postgresql = require("database_postgresql"),
        mysql = require("database_mysql"),
        db2 = require("database_db2")
    },

    replace_query = function(self, template, engine, design_name, schema_name)

        if not self.engines[engine] then
            error("Unsupported engine: " .. engine)
        end

        local cfg = self.defaults[engine]

        -- Generate schema prefix based on design name and database conventions
        -- Perfectly acceptable for schema to be empty (typical for SQLite)
        local schema_prefix = ''
        if schema_name and schema_name ~= '' and schema_name ~= '.' then
          local prefixed_name = (engine == 'db2') and schema_name:upper() or schema_name
          schema_prefix = prefixed_name .. '.'
        end

        -- Add additional placeholders to cfg for unified processing
        cfg.SCHEMA = schema_prefix
        cfg.DIALECT = self.query_dialects[engine]
        cfg.QUERY_INSERT_COLUMNS = self.query_insert_columns
        cfg.QUERY_COMMON_BOILERPLATE = self.query_common_boilerplate

        -- Lookup #27
        cfg.STATUS_INACTIVE = self.query_status.inactive
        cfg.STATUS_ACTIVE = self.query_status.active

        -- Lookup #28
        cfg.TYPE_SQL = self.query_types.system_sql
        cfg.TYPE_FORWARD_MIGRATION = self.query_types.forward_migration
        cfg.TYPE_REVERSE_MIGRATION = self.query_types.reverse_migration
        cfg.TYPE_DIAGRAM_MIGRATION = self.query_types.diagram_migration

        -- Get text block to work with
        local sql = template

        -- Apply all placeholders in unified loop
        for key, value in pairs(cfg) do
            sql = sql:gsub("${" .. key .. "}", value)
        end
        for key, value in pairs(cfg) do
            sql = sql:gsub("${" .. key .. "}", value)
        end

        return sql
    end,

    indent_sql = function(self, sql)
        -- Split into lines, strip leading and trailing whitespace from each line
        local lines = {}
        -- Use split by newline to preserve empty lines
        for line in (sql .. "\n"):gmatch("(.-)\n") do
            local stripped_line = line:match("^%s*(.-)%s*$") or line
            table.insert(lines, stripped_line)
        end

        -- Apply proper indentation with multiline string support
        local indent_unit = "    "  -- 4 spaces
        local indent_stack = {}  -- Stack to track indentation levels
        local current_indent = 0  -- Current indentation level
        local indented_lines = {}

        -- Helper functions for stack operations
        local function push_indent(level)
            table.insert(indent_stack, level)
        end

        local function pop_indent()
            if #indent_stack > 0 then
                return table.remove(indent_stack)
            end
            return 0
        end

        local function is_in_multiline_string()
            return #indent_stack > 0
        end

        local pending_opening_quote = nil
        local last_comment_position = -1  -- Track comment alignment position
        
        for i, line in ipairs(lines) do
            local processed_line = line
            local has_mlstring_start = false
            local has_mlstring_end = false
            local quote_type = nil

            -- Check for multiline string start markers
            if processed_line:find("%[=%[") then
                has_mlstring_start = true
                quote_type = "single"
            elseif processed_line:find("%[==%[") then
                has_mlstring_start = true
                quote_type = "double"
            end

            -- Check for multiline string end markers
            if processed_line:find("%]=%]") then
                has_mlstring_end = true
            elseif processed_line:find("%]==%]") then
                has_mlstring_end = true
            end

            -- Handle multiline string start
            if has_mlstring_start then
                -- Remove the multiline marker from the line
                processed_line = processed_line:gsub("%[==%[", "")
                processed_line = processed_line:gsub("%[=%[", "")
                
                -- Output the line with current indentation (the part before the marker)
                if processed_line:match("%S") then -- if there's non-whitespace content
                    local indent = indent_unit:rep(current_indent)
                    table.insert(indented_lines, indent .. processed_line)
                end
                
                -- Push current indentation level to stack and reset to zero
                push_indent(current_indent)
                current_indent = 0
                
                -- Set up the pending opening quote
                pending_opening_quote = (quote_type == "single") and "'" or '"'
                
            elseif has_mlstring_end then
                -- Pop the indentation level first
                local restored_indent = pop_indent()
                
                -- Remove the closing marker
                processed_line = processed_line:gsub("%]==%]", "")
                processed_line = processed_line:gsub("%]=%]", "")
                
                -- Append closing quote to the last line
                if #indented_lines > 0 then
                    local quote_char = '"'
                    if line:find("%]=%]") then
                        quote_char = "'"
                    end
                    indented_lines[#indented_lines] = indented_lines[#indented_lines] .. quote_char
                end
                
                -- Add any remaining content on this line after the marker
                if processed_line:match("%S") then -- if there's non-whitespace content
                    local indent = indent_unit:rep(restored_indent)
                    table.insert(indented_lines, indent .. processed_line)
                end
                
                -- Restore indentation level
                current_indent = restored_indent
            else
                -- Regular line processing
                if processed_line ~= "" then
                    -- If we have a pending opening quote, prepend it to this line
                    if pending_opening_quote then
                        processed_line = pending_opening_quote .. processed_line
                        pending_opening_quote = nil
                    end
                    
                    -- Apply indentation rules to all lines (including those in multiline strings)
                    -- If we're reducing indentation (like with ); )} )) @enduml) it applies on the current line
                    -- If we're increasing indentation (like with ( { @startuml), it applies on subsequent lines
                    if processed_line:sub(-2) == ");" or processed_line:sub(-1) == ")" or 
                       processed_line:sub(-2) == "};" or processed_line:sub(-1) == "}" or processed_line:sub(-2) == "}," or
                       processed_line:sub(-1) == "]" or processed_line:sub(-2) == "]," or 
                       processed_line:find("@enduml") then
                        -- Reducing indentation - apply to current line
                        current_indent = current_indent - 1
                        -- Ensure we don't go below 0
                        if current_indent < 0 then current_indent = 0 end
                    end
                    
                    -- Calculate current indentation level AFTER adjusting for closing brackets
                    local indent = indent_unit:rep(current_indent)

                    -- Handle comment alignment for lines with content + comments
                    local final_line = processed_line
                    local content_part, comment_part = processed_line:match("^(.-)(%s*%-%-.*)")
                    
                    if content_part and comment_part and content_part:match("%S") then
                        -- Line has content before comment
                        local content_only = content_part:gsub("%s+$", "") -- trim trailing spaces from content
                        local comment_only = comment_part:match("(%-%-.*)") -- extract just the comment part
                        
                        -- Skip alignment for HTML comments containing <!-- or -->
                        local is_html_comment = comment_only:find("<!%-%-") or comment_only:find("%-%->");
                        
                        if not is_html_comment and last_comment_position > 0 then
                            -- Try to align with previous comment position
                            local current_content_length = #(indent .. content_only)
                            local target_comment_pos = last_comment_position
                            
                            if current_content_length < target_comment_pos then
                                local spaces_needed = target_comment_pos - current_content_length
                                final_line = content_only .. string.rep(" ", spaces_needed) .. comment_only
                            else
                                -- If content is too long, establish new position with at least 2 spaces
                                final_line = content_only .. "  " .. comment_only
                                last_comment_position = current_content_length + 2
                            end
                        elseif not is_html_comment then
                            -- First comment in a new block, establish the position based on longest expected line
                            -- Use a reasonable alignment position (e.g., 40 characters from start of indent)
                            local target_pos = #indent + 40
                            local current_content_length = #(indent .. content_only)
                            
                            if current_content_length < target_pos then
                                local spaces_needed = target_pos - current_content_length
                                final_line = content_only .. string.rep(" ", spaces_needed) .. comment_only
                                last_comment_position = target_pos
                            else
                                -- Content is too long for target position, use minimum spacing
                                final_line = content_only .. "  " .. comment_only
                                last_comment_position = current_content_length + 2
                            end
                        else
                            -- HTML comment - don't align, keep original spacing
                            final_line = processed_line
                        end
                    else
                        -- No comment with content on this line, reset tracking for next comment block
                        last_comment_position = -1
                    end

                    -- Add current indentation level to the line
                    table.insert(indented_lines, indent .. final_line)

                    -- Handle opening brackets - increase indentation for subsequent lines
                    if processed_line:sub(-1) == "(" or processed_line:sub(-1) == "{" or processed_line:sub(-1) == "[" or
                       processed_line:find("@startuml") then
                        -- Increasing indentation - apply to subsequent lines
                        current_indent = current_indent + 1
                    end
                elseif not is_in_multiline_string() then
                    -- Only skip empty lines when not in multiline string
                    -- Don't add empty lines to the output
                else
                    -- In multiline string, preserve empty lines with current indentation
                    local indent = indent_unit:rep(current_indent)
                    table.insert(indented_lines, indent .. processed_line)
                end
            end
        end

        -- Remove trailing empty lines from the final output
        while #indented_lines > 0 and indented_lines[#indented_lines] == "" do
            table.remove(indented_lines)
        end

        return table.concat(indented_lines, "\n")
    end,

    run_migration = function(self, queries, engine, design_name, schema_name)
        local processed_queries = {}
        for i, q in ipairs(queries) do
            if q and q.sql then
                local formatted = string.format(q.sql, engine)
                local sql = self:replace_query(formatted, engine, design_name, schema_name)
                local indented_sql = self:indent_sql(sql)
                table.insert(processed_queries, indented_sql)
            end
        end
        return table.concat(processed_queries, "\n-- QUERY DELIMITER\n") .. "\n"
    end

}

return database