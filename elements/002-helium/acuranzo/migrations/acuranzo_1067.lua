-- Migration: acuranzo_1067.lua
-- Theme - Default

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.1 - 2025-11-28 - Reverted to inline theme definition
-- 1.0.0 - 2025-11-26 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1067"
cfg.LOOKUP_ID = "041"
cfg.LOOKUP_NAME = "Themes"
cfg.THEME_ID = "000"
cfg.THEME_NAME = "Theme-Default"
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
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID},  ${THEME_ID}, 1, '${THEME_NAME}', 0, 0, [==[
                    /* Theme-Default
                    **
                    ** Each Theme is defined as a CSS block, starting with the theme name as the CSS selector.
                    ** The entire theme is defined by way of CSS variables, which are then used to populate
                    ** other CSS rules throughout the application.
                    **
                    ** NOTE 1: To create a new Theme, clone an existing Theme and then set the ""Value"" in the
                    **         view to something unique. Then, replace the old Theme name with the new name:
                    **         1) In the Title above
                    **         2) In the main CSS selector below, after the Font Imports section
                    **         3) In the body CSS selector near the very bottom of this file
                    ** NOTE 2: CSS is largely case-sensitive, particuarly when it comes to selectors. so
                    **         be sure to use the correct case at all times.
                    ** NOTE 3: If you're editing the Theme that you are currently using, changes will be
                    **         immediately visible. This is the generally preferred way of editing themes
                    **         but be mindful not to make a change that makes the page unusable.
                    ** NOTE 4: The most likely Theme elements to change are the colors and rounding, which
                    **         are the first two sections shown. While changes can be made throughout,
                    **         most of the rules relating to the look will make use of these variables.
                    ** NOTE 5: There is a section at the bottom that is used for adding in complex background
                    **         patterns. As many of the colors are set up to be transparent, changing the
                    **         background color or pattern can dramatically impact the look of the Theme.
                    **
                    ** tt = Tooltips
                    ** bs = Bootstrap
                    ** tb = Tabulator
                    ** cm = CodeMirror
                    ** se = SunEditor
                    */


                    /* Font Imports
                    **
                    ** Webfonts, like Google Fonts, can be used but they need to be declared here at the top
                    ** of this Theme file in order to comply with browser conventions. Please comment out any
                    ** @import statements for fonts that are not going to be used. Generally speaking, there
                    ** are only two fonts - one (regular) variable-spaced font and one mono-spaced font. The
                    ** mono-spaced font is used where text editing is available, such as this CSS editor.
                    ** After adding an @import entry here, scroll down to the Fonts section and change the
                    ** appropriate font-family value to match the same name, and include a fallback in case
                    ** the font isn't available on all the systems that might access it. It would be best to
                    ** delete or comment out any fonts that aren't being used.
                    */

                    /* Examples of Monospace Fonts */
                    @import url('https://fonts.googleapis.com/css2?family=Roboto+Mono:ital,wght@0,100..700;1,100..700&display=swap');
                    /* @import url(""https://fonts.googleapis.com/css2?family=Noto+Sans+Mono&display=swap""); */
                    /* @import url('https://fonts.googleapis.com/css2?family=JetBrains+Mono:ital,wght@0,100..800;1,100..800&display=swap'); */
                    /* @import url('https://fonts.googleapis.com/css2?family=Space+Mono:ital,wght@0,400;0,700;1,400;1,700&display=swap'); */
                    /* @import url('https://fonts.googleapis.com/css2?family=Source+Code+Pro:ital,wght@0,200..900;1,200..900&display=swap'); */
                    /* @import url('https://fonts.googleapis.com/css2?family=IBM+Plex+Mono:ital,wght@0,100;0,200;0,300;0,400;0,500;0,600;0,700;1,100;1,200;1,300;1,400;1,500;1,600;1,700&display=swap'); */

                    /* Examples of Variable Fonts */
                    @import url(""https://fonts.googleapis.com/css2?family=Roboto:ital,wght@0,100;0,300;0,400;0,500;0,700;0,900;1,100;1,300;1,400;1,500;1,700;1,900&display=swap"");
                    /* @import url(""https://fonts.googleapis.com/css2?family=Cairo:ital,wght@0,300..800;1,300..800&display=swap""); */
                    /* @import url('https://fonts.googleapis.com/css2?family=Roboto+Condensed:ital,wght@0,100..900;1,100..900&display=swap'); */
                    /* @import url(""https://fonts.googleapis.com/css2?family=Open+Sans:ital,wght@0,300..800;1,300..800&display=swap""); */
                    /* @import url('https://fonts.googleapis.com/css2?family=Dosis:wght@200..800&display=swap'); */


                    .Theme-Default {

                        /* Theme Colors
                        **
                        ** Any valid CSS color identification can be used here. Some colors are deliberately
                        ** not transparent so as to hide what is underneath, such as in scrollbars or table
                        ** headers, that kind of thing. Initially, these were used to replace the standard
                        ** suite of Bootstrap names (btn-primary, etc.) but more have been added since then.
                        */
                        --ACZ-color-input: white;

                        --ACZ-color-0:  rgba(63, 63, 127, 0.25);        /* highlighting */
                        --ACZ-color-1:  rgba(63, 63, 127, 0.25);        /* highlighting */
                        --ACZ-color-2:  rgba(0, 0, 0, 0.50);            /* btn-primary, selected cells */
                        --ACZ-color-3:  rgba(100, 100, 100, 0.25);      /* btn-primary:hover */
                        --ACZ-color-4:  rgba(127, 127, 127, 0.50);      /* btn-secondary */
                        --ACZ-color-5:  rgba(127, 127, 127);              /* btn-secondary:hover */
                        --ACZ-color-6:  rgba(175, 175, 175);              /* btn-dark */
                        --ACZ-color-7:  rgba(87, 87, 87);                 /* btn-dark:hover */
                        --ACZ-color-8:  yellow;                           /* btn-warning */
                        --ACZ-color-9:  gold;                             /* btn-warning:hover */
                        --ACZ-color-10: rgba(223, 223, 223, 0.50);      /* btn-info */
                        --ACZ-color-11: rgba(223, 223, 223);              /* btn-info:hover */
                        --ACZ-color-12: rgba(255, 255, 255, 0.10);      /* zebra stripe rows */
                        --ACZ-color-13: rgba(128, 128, 128, 0.25);      /* tabulator cell hover */
                        --ACZ-color-14: rgba(   0,   0,   0, 1.00);       /* chat <pre> background, chat system text */
                        --ACZ-color-15: rgba(63,63,63,0.5);
                        --ACZ-color-16: rgba(63,63,63,1);
                        --ACZ-color-17: rgba(  96,  96, 255, 1.00);       /* scrollbar track */
                        --ACZ-color-18: rgba( 255, 255,   0, 0.70);     /* PDF crop rectangle */
                        --ACZ-color-19: rgba( 239, 239, 255, 1.00);       /* tb-cell-selected */

                        --ACZ-color-A: red;
                        --ACZ-color-B: green;
                        --ACZ-color-C: blue;
                        --ACZ-color-D: purple;
                        --ACZ-color-E: orange;
                        --ACZ-color-F: maroon;
                        --ACZ-color-G: beige;


                        /* Borders
                        **
                        ** The main choice here is likely whether the border lines should be visible at all.
                        ** replacing the color (the last part) with 'transparent' will make the border invisible
                        ** but not mess with the spacing of the elements, which is recommended.
                        **
                        ** Updating the rounding values won't impact the spacing or alignment but can dramatically
                        ** change the look of the page. The default values have been chosen to ensure a consistent
                        ** appearance when the borders are visible, particularly with many nested <div> elements.
                        */
                        --ACZ-border-1: 1px solid transparent;
                        --ACZ-border-2: 1px solid transparent;
                        --ACZ-border-3: 2px solid transparent;

                        --ACZ-border-radius-0: 10px;
                        --ACZ-border-radius-1: 8px;
                        --ACZ-border-radius-2: 6px;
                        --ACZ-border-radius-3: 4px;
                        --ACZ-border-radius-4: 5px;  /* Navigator buttons */


                        /* Fonts
                        **
                        ** Using the fonts declared via @import statements above, the other attributes
                        ** can then be defined. Most of these should not be changed as this will affect
                        ** how elements are aligned, how much room is available before text is wrapped
                        ** and that sort of thing.
                        */
                        --ACZ-font-family: 'Roboto';
                        --ACZ-font-family-mono: 'Roboto Mono';
                        --ACZ-line-height: 1.25;

                        --ACZ-font-size-1: 10px;
                        --ACZ-font-size-2: 12px;
                        --ACZ-font-size-3: 14px;  /* tb cell text */
                        --ACZ-font-size-4: 15px;
                        --ACZ-font-size-5: 20px;  /* MainTitle text */
                        --ACZ-font-size-6: 16px;  /* LTI Header */

                        --ACZ-font-size-queries: 12px;
                        --ACZ-font-size-lookups: 12px;
                        --ACZ-font-size-logs:    12px;
                        --ACZ-font-size-chats:   12px;
                        --ACZ-font-size-chats2: calc(var(--ACZ-font-size-chats) - 4px);


                        /* Tooltips
                        **
                        ** There is a separate Tippy CSS file that references these values.
                        */
                        --ACZ-tooltips-text: var(--ACZ-color-7);
                        --ACZ-tooltips-background: var(--ACZ-color-11);
                        --ACZ-tooltips-border: var(--ACZ-color-6);
                        --ACZ-tooltips-border-width: 2px;
                        --ACZ-tooltips-border-radius: var(--ACZ-border-radius-0);
                        --ACZ-tooltips-font-size: var(--ACZ-font-size-3);


                        /* Spacing and sizing
                        **
                        ** A bit of a hodgepodge of values to help with getting various elements
                        ** on the page to align consistently with others.
                        */
                        --ACZ-margin-1: 3px;
                        --ACZ-margin-2: 6px;
                        --ACZ-margin-3: 6px 12px 6px 12px; /* SunEditor Footer */

                        --ACZ-elem-height-1: 52px;
                        --ACZ-elem-height-2: 44px;
                        --ACZ-elem-height-3: 60px;
                        --ACZ-elem-height-4: 36px; /* BigButton */
                        --ACZ-elem-height-5: 25px; /* SmallButton, SunEditor footer min-height */
                        --ACZ-elem-height-6: 30px; /* RegButton */

                        --ACZ-iconimage-size: 20px;
                        --ACZ-iconimage-filter: drop-shadow(0px 0px 1px white) drop-shadow(0px 0px 1px white);


                        /* Transitions
                        **
                        ** The first number is what controls most of the fades so changing this will have the
                        ** biggest impact. The other variations are specific to when elements move or are
                        ** dragged, like sliders and that kind of thing.
                        */
                        --ACZ-transition-delay: 750ms;
                        --ACZ-transition-short: 200ms;
                        --ACZ-animation-delay-1: 300ms;
                        --ACZ-animation-delay-2: 10ms;  /* Minimal value used primarily to ensure page updates happen */
                        --ACZ-transition-std:   all var(--ACZ-transition-delay) ease;
                        --ACZ-transition-quick: all var(--ACZ-transition-delay) linear, background 100ms;
                        --ACZ-transition-login: all var(--ACZ-transition-delay) linear, width 0ms, transform 0ms;
                        --ACZ-transition-panel: all var(--ACZ-transition-delay) linear, width 0ms, transform 0ms;


                        /* Opacity/Page Mask
                        **
                        ** When modal forms like the theme selector are shown, an overlay or page mask is used
                        ** to both indicate the modal-ness as well as make it easier to view the modal. But some
                        ** themes are already pretty dark, so the mask can be adjusted to not obscure it entirely.
                        **
                        ** The other opacity values are as indicated, usually bumped up for darker themes or bumped
                        ** down for lighter themes, for the same reason.
                        */
                        --ACZ-mask-opacity: 0.8;
                        --ACZ-opacity-1: 0.50; /* text in disabled buttons */
                        --ACZ-opacity-2: 0.25; /* login button disabled */


                        /* Shadows
                        **
                        ** These are generally placed behind icons or menus, so as to separate them from the background
                        ** a little better. In particular, icons that are uploaded, like the AI Engine Icons, tend to
                        ** be pretty hard to see, so having a shadow makes them pop a little more.
                        */
                        --ACZ-shadow-1: drop-shadow(0px 0px 1px var(--ACZ-color-14));
                        --ACZ-shadow-2: drop-shadow(0px 0px 1px var(--ACZ-color-14)) drop-shadow(0px 0px 2px var(--ACZ-color-14));
                        --ACZ-shadow-3: drop-shadow(0px 0px 3px rgb(from var(--ACZ-color-14) r g b / 0.75)); /* emphasis in chats */
                        --ACZ-shadow-4: drop-shadow(1px 0px 0px #333) drop-shadow(0px 1px 0px #333) drop-shadow(-1px 0px 0px #333)  drop-shadow(0px -1px 0px #333);


                        /* Scrollbars
                        **
                        ** These appear in many places, but the underlying component that
                        ** is responsible for each scrollbar might handle them differently.
                        ** For example, CodeMirror is used as the component for managing
                        ** long pieces of syntax-highlighted text, like this Theme editor.
                        ** Tabulator is the component used to manage tables. These do not
                        ** share the same approach to managing scrollbar displays. So there
                        ** are a few more rules here than might seem necessary. But they
                        ** all should work well with the main Theme colors, so changes
                        ** here are likely more to do with fine-tuning than anything else.
                        */
                        --ACZ-scroll-width-1: 10px;
                        --ACZ-scroll-width-2: 16px;
                        --ACZ-scroll-width-3: 22px;
                        --ACZ-scroll-width-4: 18px;
                        --ACZ-scroll-width-5: 4px;

                        --ACZ-scroll-opacity-1: 0.75;
                        --ACZ-scroll-opacity-2: 1.00;
                        --ACZ-scroll-opacity-3: 0.25;

                        --ACZ-scroll-color-track:  rgb(from var(--ACZ-color-11) r g b / 0.3);
                        --ACZ-scroll-color-thumb:  rgb(from var(--ACZ-color-5 ) r g b / 0.7);
                        --ACZ-scroll-color-hover:  rgb(from var(--ACZ-color-5 ) r g b / 0.9);
                        --ACZ-scroll-color-border: rgb(from var(--ACZ-color-3 ) r g b / 0.0);

                        --ACZ-scroll-radius-1: 5px; /* General, CodeMirror overlay */
                        --ACZ-scroll-radius-2: 7px; /* Tabulator */


                        /* Progress Spinner
                        **
                        ** Lots of attention is given to this as it is a key part of the UI.
                        ** Generally speaking, no changes should be needed.
                        */
                        --ACZ-progress-color-1: var(--ACZ-color-8 ); /* active dot     */
                        --ACZ-progress-color-2: var(--ACZ-color-6 ); /* active rings   */
                        --ACZ-progress-color-3: var(--ACZ-color-5 ); /* inactive dot   */
                        --ACZ-progress-color-4: var(--ACZ-color-5 ); /* inactive rings */

                        --ACZ-progress-opacity-1: 1.0;
                        --ACZ-progress-opacity-2: 0.8;
                        --ACZ-progress-opacity-3: 0.4;
                        --ACZ-progress-opacity-4: 0.4;


                        /* Tabulator Tweaks
                        **
                        ** For the most part, Tabulator can get by using the colors for theming, but
                        ** sometimes fonts don't align very precisely, so a few extra tweaks are needed
                        ** to get things to align a little more perfectly. These help ensure that when
                        ** editing, for example, the editor is lined up in the same spot as the original.
                        */
                        --ACZ-tb-padding-left: 2px;
                        --ACZ-tb-padding-top: 3px;


                        /* CodeMirror Syntax Highlighting
                        **
                        ** CodeMirror5 is used as the text editor (like this editor used for editing CSS).
                        ** The main appeal is to have nice syntax highlighting. CodeMirror does this by
                        ** having a bunch of different colors for each kind of text element. As this can be
                        ** used for many types of code, not just CSS, it is setup with a generic set of
                        ** CSS values that can be assigned individually.
                        */
                        --ACZ-font-color-keyword:      var(--ACZ-color-9);
                        --ACZ-font-color-atom:         var(--ACZ-color-input);
                        --ACZ-font-color-number:       var(--ACZ-color-9);
                        --ACZ-font-color-def:          var(--ACZ-color-input);
                        --ACZ-font-color-variable:     var(--ACZ-color-10);
                        --ACZ-font-color-variable-2:   var(--ACZ-color-11);
                        --ACZ-font-color-operator:     var(--ACZ-color-input);
                        --ACZ-font-color-comment:      var(--ACZ-color-8);
                        --ACZ-font-color-string:       var(--ACZ-color-input);
                        --ACZ-font-color-string-2:     var(--ACZ-color-input);
                        --ACZ-font-color-meta:         var(--ACZ-color-input);
                        --ACZ-font-color-builtin:      var(--ACZ-color-input);
                        --ACZ-font-color-tag:          var(--ACZ-color-input);
                        --ACZ-font-color-attribute:    var(--ACZ-color-input);
                        --ACZ-font-color-header:       var(--ACZ-color-input);
                        --ACZ-font-color-hr:           var(--ACZ-color-input);
                        --ACZ-font-color-link:         var(--ACZ-color-input);
                        --ACZ-font-color-error:        var(--ACZ-color-input);
                        --ACZ-font-color-bracket:      var(--ACZ-color-input);
                        --ACZ-font-color-punctuation:  var(--ACZ-color-input);

                    }


                    /* Background
                    **
                    ** Some Themes have a simple color backgound while others have something
                    ** more intricate. There are plenty of CSS-only patterns that can be used
                    ** directly, just by copying & pasting the code here.
                    **
                    ** For example, here's a website with a number of background patterns.
                    ** https://freefrontend.com/css-background-patterns/
                    **
                    ** NOTE 1: The Theme name is repeated in the selector here as well as in the
                    **         first section. If the Theme doesn't appear to be working, check
                    **         that these selectors match the name of the theme exactly.
                    ** NOTE 2: This is assuming a CSS-only theme. Some themes require HTML supoprt
                    **         by way of one or more additiona <div> elements. Using these types
                    **         of themese will require adding those elements ahead of time which
                    **         will require a change to the source code. Doable, but not yet
                    **         implemented.
                    */
                    .Theme-Default body {
                        --u: 1.5vmin;
                        --c1: #8A8A8A;
                        --c2: #838383;
                        --c3: #808080;
                        --gp: 50%/calc(var(--u) * 10) calc(var(--u) * 17.4);
                        --bp: calc(var(--u) * -5) calc(var(--u) * -8.7);
                        --bg: linear-gradient(-210deg, var(--c1) 0 1.25%, #fff0 0 100%) var(--gp),
                                linear-gradient(-150deg, var(--c1) 0 1.25%, #fff0 0 100%) var(--gp),
                                conic-gradient(from 0deg at 0% 16.5%, var(--c3) 0 60deg, #fff0 0 100%) var(--gp),
                                conic-gradient(from 300deg at 100% 16.5%, var(--c2) 0 60deg, #fff0 0 100%) var(--gp),
                                conic-gradient(from 300deg at 20.75% 23.5%, var(--c1) 0 120deg, #fff0 0 100%) var(--gp),
                                conic-gradient(from 300deg at 79.25% 23.5%, var(--c1) 0 120deg, #fff0 0 100%) var(--gp),
                                conic-gradient(from 0deg at 50% 32.75%, var(--c2) 0 60deg, #fff0 0 100%) var(--gp),
                                conic-gradient(from 270deg at 20.75% 42%, var(--c2) 0 90deg, var(--c3) 0 150deg, #fff0 0 100%) var(--gp),
                                conic-gradient(from 300deg at 79.25% 42%, var(--c2) 0 60deg, var(--c3) 0 150deg, #fff0 0 100%) var(--gp),
                                conic-gradient(from 270deg at 45% 50%, var(--c2) 0 30deg, #fff0 0 100%) var(--gp),
                                conic-gradient(from 60deg at 55% 50%, var(--c3) 0 30deg, #fff0 0 100%) var(--gp),
                                linear-gradient(0deg, #f000 0 50%, var(--c1) 0 100%) var(--gp);
                        background: var(--bg), var(--bg);
                        background-position:
                                var(--bp), var(--bp), var(--bp), var(--bp), var(--bp), var(--bp), var(--bp), var(--bp), var(--bp), var(--bp),
                                var(--bp), var(--bp), 0 0, 0 0, 0 0, 0 0, 0 0, 0 0, 0 0, 0 0, 0 0, 0 0, 0 0, 0 0;
                    }
                ]==], '', '{}', ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};

        ]=]
                                                                            AS code,
        'Populate theme ${THEME_NAME} in ${TABLE} table'                    AS name,
        [=[
            # Forward Migration ${MIGRATION}: Populate theme ${THEME_NAME} in Lookup ${LOOKUP_ID} - ${LOOKUP_NAME}

            This migration adds the theme to the lookups table.
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
            WHERE lookup_id = ${LOOKUP_ID}
            AND key_idx IN (${THEME_ID});

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
