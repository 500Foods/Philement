-- Migration: acuranzo_1078.lua
-- Defaults for Lookup 045 - Client Version History

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2025-12-22 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1078"
cfg.LOOKUP_ID = "045"
cfg.LOOKUP_NAME = "Client Version History"
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_FORWARD_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            INSERT INTO ${SCHEMA}${TABLE}
            (
                lookup_id,
                key_idx,
                status_a1,
                value_txt,
                value_int,
                sort_seq,
                code,
                summary,
                collection,
                ${COMMON_FIELDS}
            )
            VALUES (
                0,                              -- lookup_id
                ${LOOKUP_ID},                   -- key_idx
                1,                              -- status_a1
                '${LOOKUP_NAME}',               -- value_txt
                0,                              -- value_int
                0,                              -- sort_seq
                '',                             -- code
                [==[
                    # ${LOOKUP_ID} - ${LOOKUP_NAME}

                    Client version history.
                ]==],                           -- summary
                ${JSON_INGEST_START}
                [==[
                    {
                        "Default": "HTMLEditor",
                        "CSSEditor": false,
                        "HTMLEditor": false,
                        "JSONEditor": true,
                        "LookupEditor": false
                    }
                ]==]
                ${JSON_INGEST_END},             -- collection
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 0, 1, '1.6.1', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 1.6.1</h3>
                    <p>This marks the first production release - where people other than the developer have been given access to test.</p>
                ]==],
                ${JIS}[==[{"Released": "2024-11-24"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 1, 1, '0.0.1', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.0.1</h3>
                    <p>This marks the official start of development for the {ACZ.CLIENT} project. It corresponds to the very first GitHub commit.
                    This is intended to be a TMS WEB Core project that will then provide the base for an AI Proxy product, an SLM product,
                    as well as many other products over time.</p><p><br></p><p><br></p>
                ]==], ${JIS}[==[{"New": true, "Focus": "Startup", "Released": "2024-03-14"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 2, 1, '0.0.2', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.0.2</h3>
                    <p>Experiment with publishing JSON rules and having GitHub manage the resulting files, combining and distributing to other repositories as needed.</p>
                ]==], ${JIS}[==[{"Focus": "JSON", "Released": "2024-03-18"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 3, 1, '0.0.3', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.0.3</h3>
                    <p>Experiment with SheetsJS for importing Excel as JSON into a TMS WEB Core app.</p>
                ]==], ${JIS}[==[{"Focus": "SheetsJS", "Released": "2024-03-24"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 4, 1, '0.1.4', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.1.4</h3>
                    <p>Start of interface design, using SheetsJS import as a test.</p>
                ]==], ${JIS}[==[{"New": true, "Focus": "MainMenu", "Released": "2024-03-26"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 5, 1, '0.1.5', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.1.5</h3>
                    <p>Work on MainMenu layout.</p>
                ]==], ${JIS}[==[{"Focus": "MainMenu", "Released": "2024-04-12"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 6, 1, '0.1.6', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.1.6</h3>
                    <p>Setup MainMenu using Tabulator table.</p>
                ]==], ${JIS}[==[{"Focus": "MainMenu", "Released": "2024-04-14"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 7, 1, '0.1.7', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.1.7</h3>
                    <p>Rearranged code into separate units, adding Tabulator.js and SunEditor.js, for example.&nbsp;</p>
                ]==], ${JIS}[==[{"Focus": "Refactoring", "Released": "2024-04-16"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 8, 1, '0.1.8', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.1.8</h3>
                    <p>Rearranged CSS into separate units.</p>
                ]==], ${JIS}[==[{"Focus": "Refactoring", "Released": "2024-04-18"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 9, 1, '0.2.0', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.2.0</h3>
                    <p>Initial Login UI work.</p>
                ]==], ${JIS}[==[{"New": true, "Focus": "Login", "Released": "2024-04-19"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 10, 1, '0.2.1', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.2.1</h3>
                    <p>More Login UI work.</p>
                ]==], ${JIS}[==[{"Focus": "Login", "Released": "2024-04-21"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 11, 1, '0.2.2', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.2.2</h3>
                    <p>Work on creating an interesting progress animation icon. This will get a lot of use, so getting it just right is important.</p>
                ]==], ${JIS}[==[{"Focus": "Progress", "Released": "2024-04-21"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 12, 1, '0.2.3', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.2.3</h3>
                    <p>Added CodeMirror instance to login page.</p>
                ]==], ${JIS}[==[{"Focus": "Login", "Released": "2024-04-22"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 13, 1, '0.2.4', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.2.4</h3>
                    <p>Incorporated a logo into the title block, work on versioning information.</p>
                ]==], ${JIS}[==[{"Focus": "Login", "Released": "2024-04-22"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 14, 1, '0.2.5', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.2.5</h3>
                    <p>Work on MainMenu, adding in sample menu data, structuring PageControls and so on.</p>
                ]==], ${JIS}[==[{"Focus": "MainMenu", "Released": "2024-04-25"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 15, 1, '0.2.6', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.2.6</h3>
                    <p>More Login UI work. Scrolling for CodeMirror, Progress icon styling.</p>
                ]==], ${JIS}[==[{"Focus": "Login", "Released": "2024-04-26"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 16, 1, '0.2.7', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.2.7</h3>
                    <p>More Login UI work. Using SimpleBar for scrollbars</p>
                ]==], ${JIS}[==[{"Focus": "Login", "Released": "2024-04-28"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 17, 1, '0.2.8', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.2.8</h3>
                    <p>Set up first CSS Theme as a separate file. Initial :root variables, that kind of thing.</p>
                ]==], ${JIS}[==[{"Focus": "Login", "Released": "2024-04-30"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 18, 1, '0.2.9', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.2.9</h3>
                    <p>Custom scrollbar work with SimpleBar.js</p>
                    <br><br>
                    <p><em>NOTE: SimpleBar has been removed in later versions in favor of more customized themed options.</em></p>
                ]==], ${JIS}[==[{"Focus": "Login", "Released": "2024-05-02"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 19, 1, '0.2.10', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.2.10</h3>
                    <p>More work on Login UI.<br><br><br></p>
                ]==], ${JIS}[==[{"Focus": "Login", "Released": "2024-05-04"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 20, 1, '0.2.11', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.2.11</h3>
                    <p>Removed Database as a login option.</p>
                ]==], ${JIS}[==[{"Focus": "Login", "Released": "2024-05-26"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 21, 1, '0.2.12', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.2.12</h3>
                    <p>Added Interact.js to the project and enabled resize+drag for the Login window.</p>
                ]==], ${JIS}[==[{"Focus": "Login", "Released": "2024-05-31"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 22, 1, '0.2.13', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.2.13</h3>
                    <p>More work on the Login screen, specifically around animation and sizing.</p>
                ]==], ${JIS}[==[{"Focus": "Login", "Released": "2024-06-12"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 23, 1, '0.2.14', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.2.14</h3>
                    <p>Prep work for adding new modules - Chat, Auditor, and Queries - as their own Delphi Units.</p>
                ]==], ${JIS}[==[{"Focus": "General", "Released": "2024-07-24"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 24, 1, '0.2.15', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.2.15</h3>
                    <ul>
                        <li>Rearranging code into separate units (UnitTabulator.js, UnitJavaScript.js, etc.)</li>
                        <li>Work on Login sequence, shading, other UI aspects</li>
                        <li>First appearance of SunEditor<br></li>
                        <li>First appearance of Tabulator<br></li>
                    </ul>
                ]==], ${JIS}[==[{"Focus": "Login", "Released": "2024-07-30"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 25, 1, '0.3.0', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.3.0</h3>
                    <ul>
                        <li>First appearance of Session Log code</li>
                        <li>Work on Login UI to show session log entries</li>
                        <li>First connection code for XData<br></li>
                        <li>First connection code for JWTs<br></li>
                    </ul>
                ]==], ${JIS}[==[{"New": true, "Focus": "SessionLog", "Released": "2024-07-31"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 26, 1, '0.3.1', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.3.1</h3>
                    <ul>
                        <li>More updates to Login sequence<br></li>
                        <li>Initializing more Application state (version info, etc.)</li>
                    </ul>
                ]==], ${JIS}[==[{"Focus": "Login", "Released": "2024-08-06"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 27, 1, '0.3.2', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.3.2</h3>
                    <ul>
                        <li>Get browser info from ip.info</li>
                        <li>Log more data about the browser environment</li>
                        <li>General Login improvements</li>
                        <li>Cleanup compiler hints</li>
                    </ul>
                ]==], ${JIS}[==[{"Focus": "Login", "Released": "2024-08-07"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 28, 1, '0.3.3', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.3.3</h3>
                    <ul>
                        <li>More Login data logging</li>
                        <li>Code refactoring</li>
                        <li>Retrieve data from the Info endpoint</li>
                    </ul>
                ]==], ${JIS}[==[{"Focus": "SessionLog", "Released": "2024-08-07"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 29, 1, '0.3.4', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.3.4</h3>
                    <ul>
                        <li>Tweaks to Login UI - default field focus, etc.</li>
                        <li>Handle possibility of AutoLogin</li>
                        <li>Load previous login credentials<br></li>
                    </ul>
                ]==], ${JIS}[==[{"Focus": "Login", "Released": "2024-08-08"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 30, 1, '0.3.5', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.3.5</h3>
                    <ul>
                        <li>Logout capability</li>
                        <li>Different kinds of Logout - Personal, Public, and Logout All</li>
                        <li>Work on UI and Logging aspects</li>
                    </ul>
                ]==], ${JIS}[==[{"Focus": "Logout", "Released": "2024-08-10"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 31, 1, '0.3.6', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.3.6</h3>
                    <ul>
                        <li>Better Logout UI</li>
                    </ul>
                ]==], ${JIS}[==[{"Focus": "Logout", "Released": "2024-08-11"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 32, 1, '0.3.7', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.3.7</h3>
                    <p>Cleanup of Login experience</p>
                    <ul>
                        <li>Fewer details in login log</li>
                        <li>Password is properly hiighlighted at along last</li>
                        <li>Better log formatting</li>
                        <li>Removed extraneous delays</li>
                        <li>Normal login now super speedy</li>
                        <li>Failed login is smoother as well</li>
                        <li>Disable most of UI while logging in</li>
                    </ul>
                ]==], ${JIS}[==[{"Focus": "Login", "Released": "2024-08-12"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 33, 1, '0.3.8', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.3.8</h3>
                    <p>More SessionLog Cleanup.</p>
                ]==], ${JIS}[==[{"Focus": "SessionLog", "Released": "2024-08-12"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 34, 1, '0.3.9', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.3.9</h3>
                    <p>Renewal cleanup</p>
                    <ul>
                        <li>Partial logs implemented to minimize log network traffic</li>
                        <li>Changed start timing of JWT renewal timer</li>
                        <li>Lots of log tweaks</li>
                        <li>Lots of theme tweaks</li>
                    </ul>
                ]==], ${JIS}[==[{"Focus": "SessionLog", "Released": "2024-08-14"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 35, 1, '0.3.10', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.3.10</h3>
                    <p>AutoLogout Feature</p>
                    <p>Work on SessionLog UI</p>
                    <ul>
                        <li>Inactivity timer, autologout, countdown timer, etc.</li>
                        <li>HackTimer.js added to handle timeout when tab not visible</li>
                    </ul>
                ]==], ${JIS}[==[{"Focus": "Logout", "Released": "2024-08-15"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 36, 1, '0.4.0', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.4.0</h3>
                    <p>First cut of Bluish Theme</p>
                ]==], ${JIS}[==[{"New": true, "Focus": "Themes", "Released": "2024-08-16"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 37, 1, '0.4.1', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.4.1</h3>
                    <p>Theme work</p>
                    <ul>
                        <li>Fiddling with CodeMirror variables to get better syntax highlighting</li>
                        <li>Fiddling with login buttons to get better layout</li>
                        <li>Just theme fiddling generally</li>
                    </ul>
                ]==], ${JIS}[==[{"Focus": "Themes", "Released": "2024-08-19"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 38, 1, '0.4.2', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.4.2</h3>
                    <p>Theme Work</p>
                    <ul>
                        <li>Added progress spinner styling to theme</li>
                        <li>Fix missing scrollbar in log view</li>
                        <li>Other tweaks to log tabulator</li>
                    </ul>
                ]==], ${JIS}[==[{"Focus": "Themes", "Released": "2024-08-19"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 39, 1, '0.4.3', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.4.3</h3>
                    <p>Theme Work</p>
                ]==], ${JIS}[==[{"Focus": "Themes", "Released": "2024-08-20"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 40, 1, '0.4.4', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.4.4</h3>
                    <p>Theme Work and work on way to display SessionLogs in different timezones</p>
                ]==], ${JIS}[==[{"Focus": "SessionLogs", "Released": "2024-08-21"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 41, 1, '0.4.5', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.4.5</h3>
                    <p>Tabulator Theming</p>
                ]==], ${JIS}[==[{"Focus": "Themes", "Released": "2024-08-22"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 42, 1, '0.4.6', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.4.6</h3>
                    <p>Tabulator scrollbar theming</p>
                ]==], ${JIS}[==[{"Focus": "Themes", "Released": "2024-08-23"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 43, 1, '0.4.7', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.4.7</h3>
                    <p>General scrollbar theming with SimpleBar</p>
                ]==], ${JIS}[==[{"Focus": "Themes", "Released": "2024-08-24"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 44, 1, '0.4.8', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.4.8</h3>
                    <p>Theme work, mostly around the setup of CSS variables</p>
                ]==], ${JIS}[==[{"Focus": "Themes", "Released": "2024-08-24"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 45, 1, '0.4.9', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.4.9</h3>
                    <p>Theme work around moving more CSS into variables. Specifically, the progress animation.</p>
                ]==], ${JIS}[==[{"Focus": "Themes", "Released": "2024-08-25"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 46, 1, '0.4.10', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.4.10</h3>
                    <p>More Theme work. Fonts and Bootstrap overrides.</p>
                ]==], ${JIS}[==[{"Focus": "Themes", "Released": "2024-08-25"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 47, 1, '0.4.11', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.4.11</h3>
                    <p>More theme work. Might seem like this is taking forever. Well, it is. But making progress.</p>
                    <ul>
                        <li>Main Menu fixes for when scaling to 300% and beyond</li>
                        <li>Adjusting icons when collapsed with the scrollbar present</li>
                        <li>Wrapping title and adjusting dbname to fit</li>
                        <li>Collapsible main menu</li>
                        <li>Added SimpleBar to main menu</li>
                        <li>First steps for theme switcher. At long last.</li>
                        </ul>
                ]==], ${JIS}[==[{"Focus": "Themes", "Released": "2024-08-25"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 48, 1, '0.5.0', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.5.0</h3>
                    <p><strong>Queries Module</strong></p>
                    <ul>
                        <li>Work on getting Queries module woven into system</li>
                        <li>This is first dynamic module</li>
                        <li>the rest are all dynamic modules</li>
                    </ul>
                    <p><strong>Themes</strong></p>
                    <ul>
                        <li>Still tweaking the themes as per usual</li>
                    </ul>
                    <p><strong>General</strong></p>
                    <ul>
                        <li>Rearranged how modules are loaded</li>
                        <li>log module is always present<br></li>
                    </ul>
                ]==], ${JIS}[==[{"New": true, "Focus": "Queries", "Released": "2024-08-26"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 49, 1, '0.5.1', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.5.1</h3>
                    <p><strong>Themes</strong></p>
                    <ul>
                        <li>Work on collapsible section transitions<br></li>
                    </ul>
                ]==], ${JIS}[==[{"Focus": "Themes", "Released": "2024-08-26"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 50, 1, '0.5.2', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.5.2</h3>
                    <p><strong>Themes</strong></p>
                    <ul>
                        <li>Added more delays to theme so various animations are more apparent</li>
                    </ul>
                    <p><strong>General</strong></p>
                    <ul>
                        <li>​Added page reload to login screen</li>
                    </ul>
                ]==], ${JIS}[==[{"Focus": "Themes", "Released": "2024-08-27"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 51, 1, '0.5.3', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.5.3</h3>
                    <p><strong>Queries</strong></p>
                    <ul>
                        <li>SQLFormatter testing (aka prettyprinting)<br></li>
                    </ul>
                ]==], ${JIS}[==[{"Focus": "Queries", "Released": "2024-08-28"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 52, 1, '0.5.4', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.5.4</h3>
                    <p><strong>Themes</strong></p>
                    <ul>
                        <li>Icon rotation when using expand/collapse mechanism<br></li>
                    </ul>
                ]==], ${JIS}[==[{"Focus": "Themes", "Released": "2024-08-28"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 53, 1, '0.5.5', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.5.5</h3>
                    <p><strong>Queries</strong></p>
                    <ul>
                        <li>Work on dynamic Module loading</li>
                    </ul>
                ]==], ${JIS}[==[{"Focus": "Queries", "Released": "2024-08-30"}]==]${JIE}, ${COMMON_VALUES}),

                (${LOOKUP_ID}, 54, 1, '0.6.0', 0, 0, '', [==[
                    <h3>{ACZ.CLIENT} Version 0.6.0</h3>
                    <p><strong>Navigator</strong></p>
                    <ul>
                        <li>First cut of Navigator - equivalent to TDBNavigator - for handing Views</li>
                        <li>Counts as a "module" only because of its importance and how much work goes into it</li>
                    </ul>
                    <p><strong>General</strong></p>
                    <ul>
                        <li>Generic AppLookup function added</li>
                        <li>More SessionLog tweaks</li>
                    </ul>
                ]==],
                ${JIS}[==[{"Focus": "Navigator", "Released": "2024-08-31"}]==]${JIE}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Populate Lookup ${LOOKUP_ID} in ${TABLE} table'                    AS name,
        [=[
            # Forward Migration ${MIGRATION}: Poulate Lookup ${LOOKUP_ID} - ${LOOKUP_NAME}

            This migration creates the lookup values for Lookup ${LOOKUP_ID} - ${LOOKUP_NAME}
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_REVERSE_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            DELETE FROM ${SCHEMA}${TABLE}
            WHERE lookup_id = 0
            AND key_idx = ${LOOKUP_ID};

            ${SUBQUERY_DELIMITER}

            DELETE FROM ${SCHEMA}${TABLE}
            WHERE lookup_id = ${LOOKUP_ID}
            AND ((key_idx >=0) AND (key_idx <= 54));

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Lookup ${LOOKUP_ID} from ${TABLE} Table'                             AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Lookup ${LOOKUP_ID} - ${LOOKUP_NAME} from ${TABLE} Table

            This is provided for completeness when testing the migration system
            to ensure that forward and reverse migrations are complete.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
