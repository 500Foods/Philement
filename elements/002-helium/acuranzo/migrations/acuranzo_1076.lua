-- Migration: acuranzo_1076.lua
-- Defaults for Lookup 043 - Tours

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.1 - 2026-03-18 - Updated to use lower-case JSON keys, <fa>-style icon definitions
-- 1.0.0 - 2025-12-21 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1076"
cfg.LOOKUP_ID = "043"
cfg.LOOKUP_NAME = "Tours"
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

                    Tours.
                ]==],                           -- summary
                ${JSON_INGEST_START}
                [==[
                    {
                        "Default": "JSONEditor",
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
                (${LOOKUP_ID},  0, 1, 'Introduction 1.0',               0, 1, '', '', ${JIS}[==[
                    {
                        "icon": "<img src=\"Logos/Acuranzo-Logo-128.png\">",
                        "steps": [
                            {
                                "text": "Hello and welcome to the {ACZ.CLIENT}!<br/>This Tour will show you some of the basic features of the app.<br/><br/>To start, the app consists of a collection of Modules, with each Module covering a particular set of Features. Each User has access to Modules and Features tailored to their role.<br/><br/>NOTE: You can also use the cursor keys to navigate the Tour.",
                                "title": "Introduction to the {ACZ.CLIENT}"
                            },
                            {
                                "text": "The left panel (or Main Panel) is where you'll find everything to do with navigating between Modules, User Preferences, and Features not specific to any one Module.<br/><br/> This panel can be resized using the slider between the left and right panels, and can be minimized using the button at the very bottom left of the pnael.",
                                "title": "Left Panel / Main Panel",
                                "element": "#divLeft",
                                "position": "right"
                            },
                            {
                                "text": "The right panel (or Module Panel) will be different for each Module, but generally contains the same elements - one or more views (tables) and editors or other tools that are specific to that Module.<br/><br/>NOTE: A Tour is available for each Module, highlighting what is different or unique or otherwise important to know. These Tours can be accessed using the <i class='fa fa-signs-post fa-lg'></i> button found at the bottom-right of each Module.",
                                "title": "Right Panel / Module Panel",
                                "element": "#divRight",
                                "position": ""
                            },
                            {
                                "text": "Back in the Main Panel, The Title Block contains the logo and the name of the app, the current version number, and when it was released.<br/><br/>Clicking anywhere in the Title Block will bring you back to this Version History Module. This Module will also be shown anytime there is a significant update to the app.",
                                "title": "Version Information",
                                "element": "#divTitle",
                                "position": "right"
                            },
                            {
                                "text": "This icon will spin whenever a network request or other long-running task is pending. For desktop users, the cursor will also spin. In most cases, you can continue working while the pending task completes.",
                                "title": "Progress Animation Icon",
                                "element": "#btnProgress",
                                "position": "bottom"
                            },
                            {
                                "text": "This indicates the current Active Database that you're logged into. Clicking the button will bring up the Datbase Selection interface, showing all the Databases that your account has access to.<br/><br/>By default, the Active Database is the last Database that you logged into. The User Profile Module has options to control this behaviour. ",
                                "title": "Active Database",
                                "element": "#btnDatabase",
                                "position": "right"
                            },
                            {
                                "text": "This is the Main Menu where the available Modules are listed for the Active Database.<br/><br/>NOTE: Access to individual Modules may be different for each Database and for each User. If a Module is not listed, please contact your Support Team or open an Issue using the <i class='fa fa-messages-question fa-lg'></i> button found at the bottom right of any Module.<br></br>For some Modules, there is a Count, typically indicating how many items are either Unread or Active, depending on the Module.",
                                "title": "Main Menu",
                                "element": "#divMainMenu",
                                "position": "right"
                            },
                            {
                                "text": "These buttons are used to access Modules and Features that are always available, regardless of the Module or Database being used.",
                                "title": "Main Menu Features",
                                "element": "#divMainMenuBottomNav",
                                "position": "top"
                            },
                            {
                                "text": "Use this button to collapse the Main Menu down to just a column of icons or expand it to the full regular display.",
                                "title": "Collapse/Expand Main Menu",
                                "element": "#btnMenuCollapse",
                                "position": "top"
                            },
                            {
                                "text": "Use this button to show the User Profile page where system preferences and other settings can be found.",
                                "title": "User Profile",
                                "element": "#btnAccount",
                                "position": "top"
                            },
                            {
                                "text": "Use this button to show the Notifications page where current and recent notifications are shown for all Modules.",
                                "title": "Notifications",
                                "element": "#btnNotifications",
                                "position": "top"
                            },
                            {
                                "text": "Use this button to show the Session Logs page.<br/><br/>A Session Log is a detailed list of everything that has taken place while using the app. Session Logs are retained for audit, technical support, and performance optimization purposes.",
                                "title": "Session Logs",
                                "element": "#btnLog",
                                "position": "top"
                            },
                            {
                                "text": "Use this button to change the app Theme. Numerous Themes are available, and Themese can be added or customized to address specifices related to branding, accessibility or other preferences.",
                                "title": "Change Theme",
                                "element": "#btnTheme",
                                "position": "top"
                            },
                            {
                                "text": "Use this button to Logout of the app. There are several Logout options depending on whether this is a private or public system. The app will automatically Logout after a period of inactivity.",
                                "title": "Logout Options",
                                "element": "#btnLogout",
                                "position": "top"
                            },
                            {
                                "text": "The right panel (or Module Panel) contains the current Module. For this Introductory Tour, we're going to look at the Version History module.<br/><br/>The Version History Module lists all of the versions of the {ACZ.CLIENT} as well as the {ACZ.SERVER}. Release notes are also included, highlighting any relevant changes, new Modules, and so on.",
                                "title": "Version History Module",
                                "element": "#divRight",
                                "position": ""
                            },
                            {
                                "text": "The top section of a typical Module contains Options that apply to the Module overall.",
                                "title": "Version History Options",
                                "element": "#divVersionsTop",
                                "position": "bottom"
                            },
                            {
                                "text": "Clicking the Toolbox button will show a menu listing all the configurable Options for the Module. Select an item from the Toolbox to configure these Options.",
                                "title": "Version History Toolbox",
                                "element": "#btnVersionsToolbox",
                                "position": "right"
                            },
                            {
                                "text": "Clicking the History button will show the last editor of the currently selected Module item. Clicking the History button will show the Change History or Audit History of the item, if available.",
                                "title": "Version History History",
                                "element": "#btnVersionsHistory",
                                "position": "left"
                            },
                            {
                                "text": "The bottom section of a Module contains supplemental features. This is where any linked Reports can be found, if available. There aren't any Reports for the Version History Module at this time.",
                                "title": "Version History Features",
                                "element": "#divVersionsBottom",
                                "position": "top"
                            },
                            {
                                "text": "Clicking the Cleanup button will refresh the Module contents. This may involve reloading data from the Server, if applicable, or removing any filters or changes from the defaul settings for the Module.",
                                "title": "Version History Cleanup",
                                "element": "#btnVersionsRefresh",
                                "position": "left"
                            },
                            {
                                "text": "Clicking this icon will bring up the Tour for the Module. The Tour is also presented when opening a Module for the first time as well as when the Tour is updated.",
                                "title": "Version History Tour",
                                "element": "#btnVersionsGuide",
                                "position": "right"
                            },
                            {
                                "text": "Clicking this icon will open up the {ACZ.CLIENT} Documentation and locate the specific section for the Module.",
                                "title": "Version History Documentation",
                                "element": "#btnVersionsDocs",
                                "position": "left"
                            },
                            {
                                "text": "Clicking this icon will show a form that will allow you to search for past Issues related to this Module, or to register a new Issue.",
                                "title": "Version History Issues",
                                "element": "#btnVersionsIssue",
                                "position": "left"
                            },
                            {
                                "text": "Most Modules have one or more Views (Tables). Each view is made up of a Header, a set of records, a footer, and a Navigator. Often, records have Columns that are sorted, filtered, or grouped to organize the data in a specific way, all of which can typically be changed.",
                                "title": "Version History View",
                                "element": "#divVersions",
                                "position": "right"
                            },
                            {
                                "text": "The Header portion of the View shows the column names as well as any sorting or filtering buttons. Multiple Columns can be sorted by shift-clicking on the sort icons. Hovering over the Column Title will show Tooltips with more descriptive names. This is particularly useful when the Column Title is just an icon.",
                                "title": "View Headers",
                                "element": "#divVersions > div:nth-child(1)",
                                "position": "bottom"
                            },
                            {
                                "text": "Most Views will have a Column Chooser icon. Use this to bring up a menu where additional Columns can be shown, or existing columns can be hidden. This is also where Column grouping options are found.",
                                "title": "View Headers - Column Chooser",
                                "element": "#divVersions > div:nth-child(1) > div:nth-child(1) > div:nth-child(1) > div:nth-child(1)",
                                "position": "right"
                            },
                            {
                                "text": "The Navigator that appears at the bottom of most Views contains as many as four sections. Buttons within each section will be disabled or even hidden entirely in some cases, depending on which record is selected or whether the view is read-only.",
                                "title": "View Navigator",
                                "element": "#divVersionsNav",
                                "position": "top"
                            },
                            {
                                "text": "Of special note, there is a View Options button. Clicking this button will bring up a menu with options for adjusting the size of the view, removing filters,  hiding parts of the Navigator, and more.",
                                "title": "View Navigator Options",
                                "element": "#btnVersionsNavOptions",
                                "position": "top"
                            },
                            {
                                "text": "Use the Search feature of views to quickly find the records you're looking for. Entering search terms here will filter the list of records to include only those related to the search term.<br/><br/>NOTE: This is a comprehensive search, meaning that all the fields are searched, including those that are not shown in the View. In some cases, the search extends to other linked records as well.",
                                "title": "View Navigator Search",
                                "element": "#divVersionsNav4",
                                "position": "top"
                            },
                            {
                                "text": "While most Modules have or more Views, they also have one or more Pages where additional data can be either viewed or edited. In the Version History Module, this is used to display the detailed list of changes.",
                                "title": "Version History Content",
                                "element": "#divVersion",
                                "position": "left"
                            },
                            {
                                "text": "The Module Toolbar contains Features for navigating between different Pages, as well as Features related to an individual record that has been highlighted in the View.<br/><br/>In the Version History Module, selecting a record in the View will bring up the detailed list of changes for that specific Version.",
                                "title": "Version History Toolbar",
                                "element": "#divVersionTools",
                                "position": "bottom"
                            },
                            {
                                "text": "Most Modules have one or more Collapse View buttons. These are used to hide or show the View. This can be used, for example, to give more room for editing a larger document, or to reduce a bit of the clutter on the screen.",
                                "title": "Collapse View",
                                "element": "#btnVersionsCollapse",
                                "position": "bottom"
                            },
                            {
                                "text": "Many Modules are organized to have multiple Pages or multiple Editors, or in the case of the Version History Module, multiple sets of data to manage. In these cases, a Selector can be used to switch between the Pages or Editors.",
                                "title": "Page Selector",
                                "element": "#divVersionsSelector",
                                "position": "bottom"
                            },
                            {
                                "text": "And finally, additional Tools can be found here. In the Versions History Module, these are just Tools for exporting the detailed list of changes. In other Modules, you might find things like Code Formatters or buttons to change the size of the fonts used.",
                                "title": "Additional Tools",
                                "element": "#divVersionsExport",
                                "position": "bottom"
                            },
                            {
                                "text": "Congratulations!<br/><br/>You made it to the end of the Tour. Nicely done. Be sure to check out the Tours for the other Modules at your earliest convenience. And if you have any questions or would like to report a problem of any kind, please use the Issues button at the bottom-right of each Module.",
                                "title": "Introductory Tour Complete",
                                "element": "document.body",
                                "position": ""
                            }
                        ]
                    }
                ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  1, 1, 'Session Logs 1.0',               0, 1, '', '', ${JIS}[==[
                    {
                        "icon": "<fa fa-receipt></fa>",
                        "steps": [
                            {
                            "text": "This Tour will help get you acquainted with the Sessions Log module.",
                            "title": "Session Logs Module"
                            }
                        ]
                    }
                ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  2, 1, 'Queries Module 1.0',             0, 1, '', '', ${JIS}[==[
                    {
                        "icon": "<fa fa-clipboard-question></fa>",
                        "steps": [
                            {
                            "text": "This Tour will help get you acquainted with the Queries module. This module is used to manage both SQL queries and AI Prompts, along with some tools to help with testing and validation.",
                            "title": "Queries Module"
                            }
                        ]
                    }
                ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  3, 1, 'Lookups Module 1.0',             0, 1, '', '', ${JIS}[==[
                    {
                        "icon": "<fa fa-clipboard-list></fa>",
                        "steps": [
                            {
                                "text": "This Module is used to view or edit various app Lookups. <br/><br/>NOTE: Access to view or edit individual Lookups is restricted. Contact your Support Team if you need access to more than what is available to you currently..",
                                "title": "Lookups Module"
                            },
                            {
                                "text": "This View shows the Lookups that are available to you. Selecting a Lookup from this List will populate the next View with its Values.",
                                "title": "Lookups List",
                                "element": "#divLookupsHolder",
                                "position": "right"
                            },
                            {
                                "text": "The View shows the Values for the Lookup selected from the View to the left. Selecting a Value from this View will populate the right pane with its contents.",
                                "title": "Lookup Values",
                                "element": "#divLookupHolder",
                                "position": "right"
                            },
                            {
                                "text": "Each Lookup Value can contain additional information in the form of either HTML, JSON, or CSS. This Panel contains editors for each. <br/><br/>NOTE: Be sure to click the <i class='fa fa-check fa-lg'></i> button in the Navigator at the bottom of the Values View to save any changes made in these editors.",
                                "title": "Lookup Editors",
                                "element": "#divLookupKeyHolder",
                                "position": "left"
                            },
                            {
                                "text": "This Toolbar contains buttons that apply primarily to the selected Lookup Value.",
                                "title": "Lookup Value Toolbar",
                                "element": "#divLookupsTools",
                                "position": "left"
                            },
                            {
                                "text": "Use this button to show or hide the Lookups List View. This can be helpful if more screen space is needed to edit a particular Lookup Value.",
                                "title": "Show/Hide Lookups List View",
                                "element": "#btnLookupsCollapse",
                                "position": "bottom"
                            },
                            {
                                "text": "Use this button to show or hide the Lookup Values View. This can be helpful if more screen space is needed to edit a particular Lookup Value.",
                                "title": "Show/Hide Lookup Values View",
                                "element": "#btnLookupCollapse",
                                "position": "bottom"
                            },
                            {
                                "text": "These buttons are used to switch between the available Lookup Value Editors. Some Lookup Values have just one Editor available, whereas others have all of the editors available. Also, some Lookup Values will automatically switch to the most appropriate editor.",
                                "title": "Lookup Editor Selector",
                                "element": "#divLookupsSelector",
                                "position": "bottom"
                            },
                            {
                                "text": "Use these buttons to import or export the content for a particular Lookup Value. The Lookup Value can be printed, e-mailed, downloaded, or uploaded.",
                                "title": "Import/Export Lookup Value",
                                "element": "#divLookupsExport",
                                "position": "bottom"
                            },
                            {
                                "text": "Use this button to change the font size for a particular editor. <br/><br/>NOTE: This feature may not be available for all editors.",
                                "title": "Change Font Size",
                                "element": "#divLookupsSize",
                                "position": "bottom"
                            },
                            {
                                "text": "Use this button to 'beautify' code, typically used only for the CSS Editor. This will reformat the content using consistent indentation and so on.",
                                "title": "PrettyPrint Code",
                                "element": "#btnLookupsToolsFormat",
                                "position": "bottom"
                            },
                            {
                                "text": "This Toolbar contains that apply to the Lookups Module overall.",
                                "title": "Lookups Module Tools",
                                "Eeement": "#divLookupsBottom",
                                "position": "top"
                            },
                            {
                                "text": "Use this button to reload all of the Views and refresh the display.",
                                "title": "Lookups Cleanup",
                                "element": "#btnLookupsRefresh",
                                "position": "top"
                            },
                            {
                                "text": "Use this button to show this Tour.",
                                "title": "Lookups Tour",
                                "element": "#btnLookupsGuide",
                                "position": "top"
                            },
                            {
                                "text": "Use this button to view the documentation for the Lookups Module.",
                                "title": "Lookups Documentation",
                                "element": "#btnLookupsDocs",
                                "position": "top"
                            },
                            {
                                "text": "Use this button to create a new Issue or view existing Issues related to the Lookups Module.",
                                "title": "Lookups Issues",
                                "element": "#btnLookupsIssue",
                                "position": "top"
                            }
                        ]
                    }
                ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  4, 1, 'AI Chats Module 1.0',            0, 1, '', '', ${JIS}[==[
                    {
                        "icon": "<fa fa-robot></fa>",
                        "steps": [
                            {
                                "text": "This Module is used to facilite chat with various AI chatbots, while also maintaining a history of prior chats. <br/><br/>NOTE: All chats are automatically logged and audited.",
                                "title": "AI Chats Module"
                            },
                            {
                                "text": "This View shows the Lookups that are available to you. Selecting a Lookup from this List will populate the next View with its Values.",
                                "title": "Lookups List",
                                "element": "#divLookupsHolder",
                                "position": "right"
                            },
                            {
                                "text": "The View shows the Values for the Lookup selected from the View to the left. Selecting a Value from this View will populate the right pane with its contents.",
                                "title": "Lookup Values",
                                "element": "#divLookupHolder",
                                "position": "right"
                            },
                            {
                                "text": "Each Lookup Value can contain additional information in the form of either HTML, JSON, or CSS. This Panel contains editors for each. <br/><br/>NOTE: Be sure to click the <i class='fa fa-check fa-lg'></i> button in the Navigator at the bottom of the Values View to save any changes made in these editors.",
                                "title": "Lookup Editors",
                                "element": "#divLookupKeyHolder",
                                "position": "left"
                            },
                            {
                                "text": "This Toolbar contains buttons that apply primarily to the selected Lookup Value.",
                                "title": "Lookup Value Toolbar",
                                "element": "#divLookupsTools",
                                "position": "left"
                            },
                            {
                                "text": "Use this button to show or hide the Lookups List View. This can be helpful if more screen space is needed to edit a particular Lookup Value.",
                                "title": "Show/Hide Lookups List View",
                                "element": "#btnLookupsCollapse",
                                "position": "bottom"
                            },
                            {
                                "text": "Use this button to show or hide the Lookup Values View. This can be helpful if more screen space is needed to edit a particular Lookup Value.",
                                "title": "Show/Hide Lookup Values View",
                                "element": "#btnLookupCollapse",
                                "position": "bottom"
                            },
                            {
                                "text": "These buttons are used to switch between the available Lookup Value Editors. Some Lookup Values have just one Editor available, whereas others have all of the editors available. Also, some Lookup Values will automatically switch to the most appropriate editor.",
                                "title": "Lookup Editor Selector",
                                "element": "#divLookupsSelector",
                                "position": "bottom"
                            },
                            {
                                "text": "Use these buttons to import or export the content for a particular Lookup Value. The Lookup Value can be printed, e-mailed, downloaded, or uploaded.",
                                "title": "Import/Export Lookup Value",
                                "element": "#divLookupsExport",
                                "position": "bottom"
                            },
                            {
                                "text": "Use this button to change the font size for a particular editor. <br/><br/>NOTE: This feature may not be available for all editors.",
                                "title": "Change Font Size",
                                "element": "#divLookupsSize",
                                "position": "bottom"
                            },
                            {
                                "text": "Use this button to 'beautify' code, typically used only for the CSS Editor. This will reformat the content using consistent indentation and so on.",
                                "title": "PrettyPrint Code",
                                "element": "#btnLookupsToolsFormat",
                                "position": "bottom"
                            },
                            {
                                "text": "This Toolbar contains that apply to the Lookups Module overall.",
                                "title": "Lookups Module Tools",
                                "element": "#divLookupsBottom",
                                "position": "top"
                            },
                            {
                                "text": "Use this button to reload all of the Views and refresh the display.",
                                "title": "Lookups Cleanup",
                                "element": "#btnLookupsRefresh",
                                "position": "top"
                            },
                            {
                                "text": "Use this button to show this Tour.",
                                "title": "Lookups Tour",
                                "element": "#btnLookupsGuide",
                                "position": "top"
                            },
                            {
                                "text": "Use this button to view the documentation for the Lookups Module.",
                                "title": "Lookups Documentation",
                                "element": "#btnLookupsDocs",
                                "position": "top"
                            },
                            {
                                "text": "Use this button to create a new Issue or view existing Issues related to the Lookups Module.",
                                "title": "Lookups Issues",
                                "element": "#btnLookupsIssue",
                                "position": "top"
                            }
                        ]
                    }
                ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  5, 1, 'AI Auditor Module 1.0',          0, 1, '', '', ${JIS}[==[
                    {
                        "icon": "<fa fa-receipt></fa>",
                        "steps": [
                            {
                                "text": "This Tour will help get you acquainted with the AI Auditor module.",
                                "title": "AI Auditor Module"
                            }
                        ]
                    }
                ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  6, 1, 'Document Library Module 1.0',    0, 1, '', '', ${JIS}[==[
                    {
                        "icon": "<fa fa-feather></fa>",
                        "steps": [
                            {
                                "text": "This Tour will help get you acquainted with the Document Library module.",
                                "title": "Doucment Library Module"
                            }
                        ]
                    }
                ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  7, 1, 'Reports Module 1.0',             0, 1, '', '', ${JIS}[==[
                    {
                        "icon": "<fa fa-clipboard-check></fa>",
                        "steps": [
                            {
                                "text": "This Tour will help get you acquainted with the Reports module.",
                                "title": "Reports Module"
                            }
                        ]
                    }
                ]==]${JIE}, ${COMMON_VALUES});

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
            AND key_idx IN (0,1,2,3,4,5,6,7);

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
