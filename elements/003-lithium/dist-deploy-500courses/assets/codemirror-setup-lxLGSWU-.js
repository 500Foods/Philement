import { V as ViewPlugin, a as EditorView, k as keymap, S as StateField, b as StateEffect, c as EditorSelection, s as syntaxTree, f as foldable, d as foldEffect, e as unfoldAll, C as Compartment, l as lineNumbers, g as foldGutter, h as foldKeymap, i as highlightActiveLineGutter, j as highlightSpecialChars, m as drawSelection, n as highlightActiveLine, o as indentOnInput, E as EditorState, p as history, q as defaultKeymap, t as historyKeymap, v as oneDark, w as bracketMatching, x as highlightWhitespace, y as highlightSelectionMatches, z as toggleLineComment, A as toggleBlockComment, T as Transaction, B as foldAll, D as foldCode, F as Facet, G as unfoldCode, H as undoDepth, I as redoDepth, J as Decoration, W as WidgetType, K as sql, L as html, M as javascript, N as markdown, O as css, P as json } from './codemirror-Bg1rM8n2.js';

const scrollbarTheme = EditorView.theme({
  // === Native scrollbar suppression ===
  "& .cm-editor": {
    scrollbarWidth: "none",                    // Firefox
    msOverflowStyle: "none",                   // IE/Edge
  },
  "& .cm-editor::-webkit-scrollbar": {
    display: "none",                           // Chrome/Safari/Edge
  },
  "& .cm-scroller": {
    scrollbarWidth: "none",                    // Firefox
    msOverflowStyle: "none",                   // IE/Edge
    overflow: "auto !important",               // Allow scrolling but hide native bars
  },
  "& .cm-scroller::-webkit-scrollbar": {
    display: "none",                           // Chrome/Safari/Edge
  },
  "& .cm-content": {
    scrollbarWidth: "none",                    // Firefox
    msOverflowStyle: "none",                   // IE/Edge
  },
  "& .cm-content::-webkit-scrollbar": {
    display: "none",                           // Chrome/Safari/Edge
  },
  "& .cm-gutters": {
    scrollbarWidth: "none",                    // Firefox
    msOverflowStyle: "none",                   // IE/Edge
  },
  "& .cm-gutters::-webkit-scrollbar": {
    display: "none",                           // Chrome/Safari/Edge
  },

  // === Lithium custom scrollbars ===
  // Content padding to prevent overlap with scrollbars
  "& .cm-scroller[data-scrollbars='vertical'] .cm-content": {
    paddingRight: "11px",                      // 12px scrollbar + 2px spacing
  },
  "& .cm-scroller[data-scrollbars='horizontal'] .cm-content": {
    paddingBottom: "11px",                     // 12px scrollbar + 2px spacing
  },
  "& .cm-scroller[data-scrollbars='both'] .cm-content": {
    paddingRight: "11px",                      // 12px scrollbar + 2px spacing
    paddingBottom: "11px",                     // 12px scrollbar + 2px spacing
  },
  "& .cm-scroller-corner": {
    background: "var(--bg-secondary)",         // Match track background
    zIndex: "15",                              // Same z-index as tracks
    pointerEvents: "none",                     // No interaction
  },
  "& .cm-scroller-track-v": {
    top: "0",
    right: "0",
    width: "9px",                             // track width
    height: "100%",
    pointerEvents: "auto",
  },
  "& .cm-scroller-track-h": {
    bottom: "0",
    height: "9px",
    pointerEvents: "auto",
  },

  "& .cm-scroller-thumb": {
    position: "absolute",
    background: "var(--accent-alt-primary)",   // Lithium accent color
    borderRadius: "var(--border-radius-full)",
    cursor: "pointer",
    transition: "background-color 0.3s ease, border 0.3s ease"
  },
  "& .cm-scroller-thumb-v": {
    width: "6px",                              // thumb width
    left: "1px",                               // centers the 8px thumb in 12px track
    top: "2px",                                // 2px from top edge
    bottom: "2px",                             // 2px from bottom edge
  },
  "& .cm-scroller-thumb-h": {
    height: "6px",                             // thumb height
    left: "2px",                               // 2px from left edge
    right: "2px",                              // 2px from right edge
    top: "1px",                                // centers the 8px thumb in 12px track
  },

  // Hover state
  "& .cm-scroller-thumb:hover": {
    background: "var(--accent-primary)",       // brighter accent on hover
    border: "1px solid var(--accent-primary)",
  },
});

class CustomScrollbars {
  constructor(view) {
    this.view = view;
    this.scroller = view.scrollDOM;

    // Make sure scroller can contain absolute children
    const style = getComputedStyle(this.scroller);
    if (style.position === "static") {
      this.scroller.style.position = "relative";
    }

    // Create vertical scrollbar
    this.vTrack = this.createTrack("v");
    this.vThumb = this.createThumb("v");
    this.vTrack.appendChild(this.vThumb);
    this.scroller.appendChild(this.vTrack);

    // Create horizontal scrollbar
    this.hTrack = this.createTrack("h");
    this.hThumb = this.createThumb("h");
    this.hTrack.appendChild(this.hThumb);
    this.scroller.appendChild(this.hTrack);

    // Create corner element (shown when both scrollbars are visible)
    this.corner = this.createCorner();
    this.scroller.appendChild(this.corner);

    // Event listeners
    this.scroller.addEventListener("scroll", this.onScroll.bind(this), { passive: true });
    this.setupDrag(this.vThumb, "vertical");
    this.setupDrag(this.hThumb, "horizontal");
    this.setupTrackClick(this.vTrack, "vertical");
    this.setupTrackClick(this.hTrack, "horizontal");

    // Observe size changes
    this.resizeObserver = new ResizeObserver(() => this.updateBars());
    this.resizeObserver.observe(this.scroller);

    // Observe window resize for fixed positioning updates
    this.windowResizeHandler = () => this.updateBars();
    window.addEventListener('resize', this.windowResizeHandler);

    this.updateBars();
  }

  createTrack(axis) {
    const track = document.createElement("div");
    track.className = `cm-scroller-track cm-scroller-track-${axis}`;
    return track
  }

  createThumb(axis) {
    const thumb = document.createElement("div");
    thumb.className = `cm-scroller-thumb cm-scroller-thumb-${axis}`;
    return thumb
  }

  createCorner() {
    const corner = document.createElement("div");
    corner.className = "cm-scroller-corner";
    return corner
  }

  updateBars() {
    const rect = this.scroller.getBoundingClientRect();
    const { scrollHeight, clientHeight, scrollTop, scrollWidth, clientWidth, scrollLeft } = this.scroller;

    // Get gutter width to position horizontal scrollbar correctly
    const gutters = this.scroller.querySelector('.cm-gutters');
    const gutterWidth = gutters ? gutters.offsetWidth : 0;

    // Check if both scrollbars will be visible for corner padding
    const needsVertical = scrollHeight > clientHeight + 1;
    const needsHorizontal = scrollWidth > clientWidth + 1;

    // Update data attribute for CSS content padding
    if (needsVertical && needsHorizontal) {
      this.scroller.setAttribute('data-scrollbars', 'both');
    } else if (needsVertical) {
      this.scroller.setAttribute('data-scrollbars', 'vertical');
    } else if (needsHorizontal) {
      this.scroller.setAttribute('data-scrollbars', 'horizontal');
    } else {
      this.scroller.removeAttribute('data-scrollbars');
    }

    // Vertical scrollbar (fixed to right edge of scroller)
    this.vTrack.style.display = needsVertical ? "block" : "none";
    this.vTrack.style.position = "fixed";
    this.vTrack.style.top = `${rect.top}px`;
    this.vTrack.style.right = `${Math.trunc(window.innerWidth - rect.right)}px`;
    this.vTrack.style.height = `${rect.height}px`;
    // Add bottom padding if horizontal scrollbar is visible
    if (needsHorizontal) {
      this.vTrack.style.height = `${rect.height - 9}px`;
    }

    if (needsVertical) {
      const thumbHeight = Math.max(24, (clientHeight / scrollHeight) * clientHeight);
      // Available track height minus padding (2px top + 2px bottom) and corner adjustment
      const availableTrackHeight = clientHeight - 4 - (needsHorizontal ? 9 : 0);
      const maxThumbTop = availableTrackHeight - thumbHeight;
      const thumbTop = (scrollTop / (scrollHeight - clientHeight)) * maxThumbTop;

      this.vThumb.style.height = `${thumbHeight}px`;
      this.vThumb.style.top = `${Math.max(2, Math.min(thumbTop + 2, maxThumbTop + 2))}px`;
    }

    // Horizontal scrollbar (fixed to bottom, starts after gutters)
    this.hTrack.style.display = needsHorizontal ? "block" : "none";
    this.hTrack.style.position = "fixed";
    this.hTrack.style.bottom = `${Math.trunc(window.innerHeight - rect.bottom)}px`;
    this.hTrack.style.left = `${rect.left + gutterWidth}px`;
    const contentWidth = clientWidth - gutterWidth;
    this.hTrack.style.width = `${contentWidth}px`;
    // Add right padding if vertical scrollbar is visible
    if (needsVertical) {
      this.hTrack.style.width = `${contentWidth - 9}px`;
    }

    if (needsHorizontal) {
      const thumbWidth = Math.max(18, (contentWidth / scrollWidth) * contentWidth);
      // Available track width minus padding (2px left + 2px right) and corner adjustment
      const availableTrackWidth = contentWidth - 4 - (needsVertical ? 9 : 0);
      const maxThumbLeft = availableTrackWidth - thumbWidth;
      const thumbLeft = (scrollLeft / (scrollWidth - clientWidth)) * maxThumbLeft;

      this.hThumb.style.width = `${thumbWidth}px`;
      this.hThumb.style.left = `${Math.max(2, Math.min(thumbLeft + 2, maxThumbLeft + 2))}px`;
    }

    // Corner element (fills the gap when both scrollbars are visible)
    this.corner.style.display = (needsVertical && needsHorizontal) ? "block" : "none";
    if (needsVertical && needsHorizontal) {
      this.corner.style.position = "fixed";
      this.corner.style.bottom = `${Math.trunc(window.innerHeight - rect.bottom)}px`;
      this.corner.style.right = `${Math.trunc(window.innerWidth - rect.right)}px`;           
      this.corner.style.width = "9px";
      this.corner.style.height = "9px";
    }
  }

  onScroll() {
    this.updateBars();
  }

  setupTrackClick(track, axis) {
    track.addEventListener("pointerdown", (e) => {
      // Don't handle clicks on the thumb itself
      if (e.target !== track) return

      e.preventDefault();
      const rect = track.getBoundingClientRect();
      const { scrollHeight, clientHeight, scrollWidth, clientWidth } = this.scroller;

      if (axis === "vertical") {
        const scrollable = scrollHeight - clientHeight;
        const clickY = e.clientY - rect.top;
        const trackHeight = rect.height;
        const scrollRatio = clickY / trackHeight;
        this.scroller.scrollTop = scrollRatio * scrollable;
      } else {
        const scrollable = scrollWidth - clientWidth;
        const clickX = e.clientX - rect.left;
        const trackWidth = rect.width;
        const scrollRatio = clickX / trackWidth;
        this.scroller.scrollLeft = scrollRatio * scrollable;
      }
    });
  }

  setupDrag(thumb, axis) {
    let startPos = 0;
    let startScroll = 0;

    const onPointerMove = (e) => {
      e.preventDefault();
      const delta = axis === "vertical" ? e.clientY - startPos : e.clientX - startPos;
      const { scrollHeight, clientHeight, scrollWidth, clientWidth } = this.scroller;

      // Check if both scrollbars are visible for proper constraints
      const needsVertical = scrollHeight > clientHeight + 1;
      const needsHorizontal = scrollWidth > clientWidth + 1;

      if (axis === "vertical") {
        const scrollable = scrollHeight - clientHeight;
        // Thumb scrollable area accounts for padding and corner constraints
        let thumbScrollable = clientHeight - thumb.offsetHeight - 4; // 2px top + 2px bottom
        if (needsHorizontal) {
          thumbScrollable -= 9; // Additional constraint when horizontal scrollbar is visible
        }
        const newScroll = startScroll + (delta / thumbScrollable) * scrollable;
        this.scroller.scrollTop = Math.max(0, Math.min(newScroll, scrollable));
      } else {
        const scrollable = scrollWidth - clientWidth;
        // Thumb scrollable area accounts for padding and corner constraints
        let thumbScrollable = clientWidth - thumb.offsetWidth - 4; // 2px left + 2px right
        if (needsVertical) {
          thumbScrollable -= 9; // Additional constraint when vertical scrollbar is visible
        }
        const newScroll = startScroll + (delta / thumbScrollable) * scrollable;
        this.scroller.scrollLeft = Math.max(0, Math.min(newScroll, scrollable));
      }
    };

    const onPointerUp = () => {
      document.removeEventListener("pointermove", onPointerMove);
      document.removeEventListener("pointerup", onPointerUp);
      thumb.style.pointerEvents = "auto";
    };

    thumb.addEventListener("pointerdown", (e) => {
      e.preventDefault();
      e.stopPropagation();

      startPos = axis === "vertical" ? e.clientY : e.clientX;
      startScroll = axis === "vertical" ? this.scroller.scrollTop : this.scroller.scrollLeft;

      // Adjust for thumb padding offset
      if (axis === "vertical") {
        const thumbTop = parseFloat(thumb.style.top) || 0;
        startPos -= thumbTop - 2; // Remove the 2px top offset
      } else {
        const thumbLeft = parseFloat(thumb.style.left) || 0;
        startPos -= thumbLeft - 2; // Remove the 2px left offset
      }

      thumb.style.pointerEvents = "none"; // prevent text selection issues
      document.addEventListener("pointermove", onPointerMove);
      document.addEventListener("pointerup", onPointerUp, { once: true });
    });
  }

  update(update) {
    if (update.docChanged || update.viewportChanged || update.geometryChanged) {
      // Use requestAnimationFrame to avoid layout thrashing
      requestAnimationFrame(() => this.updateBars());
    }
  }

  destroy() {
    this.resizeObserver.disconnect();
    this.scroller.removeEventListener("scroll", this.onScroll);
    window.removeEventListener('resize', this.windowResizeHandler);
    this.vTrack.remove();
    this.hTrack.remove();
    this.corner.remove();
  }
}

function customScrollbars() {
  return [
    scrollbarTheme,
    ViewPlugin.fromClass(CustomScrollbars)
  ]
}

/**
 * Virtual Column Cursor Extension for CodeMirror 6
 *
 * This extension brings back the classic "Borland IDE / old-school editor" cursor behavior
 * that many people miss in modern editors.
 *
 * - When you press ↑ or ↓, the cursor tries to stay in the **same column**,
 *   even if the target line is shorter or completely empty.
 * - If the target line is too short, it **temporarily adds spaces** so the cursor
 *   can actually sit at your desired column.
 * - As soon as you leave that line (by moving up, down, or clicking elsewhere),
 *   the trailing spaces are **automatically trimmed**.
 *
 * This makes aligning comments, text, or code extremely easy in monospace fonts,
 * without permanently filling your files with trailing whitespace.
 *
 * It's the same idea as the "fake-virtual-space" plugins people use in VS Code.
 *
 * @module core/cm6-virtual-columns
 */


/**
 * Get the visual column position of a character position, accounting for tabs.
 * @param {EditorState} state - The editor state
 * @param {number} pos - The character position
 * @returns {number} The visual column (0-based)
 */
function getVisualColumn(state, pos) {
  const line = state.doc.lineAt(pos);
  const textBefore = line.text.slice(0, pos - line.from);
  let col = 0;
  const tabSize = state.tabSize;
  for (const ch of textBefore) {
    col += ch === '\t' ? tabSize - (col % tabSize) : 1;
  }
  return col;
}

/**
 * Get the padding string needed to reach the desired column.
 * @param {EditorState} state - The editor state
 * @param {number} lineLength - The current length of the line
 * @param {number} desiredColumn - The desired visual column
 * @returns {string} Spaces to pad the line
 */
function getPadding(state, lineLength, desiredColumn) {
  const needed = Math.max(0, desiredColumn - lineLength);
  return ' '.repeat(needed);
}

/**
 * Move the cursor vertically, maintaining virtual column position.
 * @param {EditorView} view - The editor view
 * @param {number} direction - Direction to move (-1 for up, 1 for down)
 * @param {boolean} extendSelection - Whether to extend the selection
 * @returns {boolean} Whether the move was successful
 */
function moveVertically(view, direction, extendSelection) {
  const state = view.state;
  const sel = state.selection.main;
  const currentLine = state.doc.lineAt(sel.head);
  const currentCol = getVisualColumn(state, sel.head);

  // Get or initialize desired column
  let desired = state.field(desiredColumnField, false);
  if (desired === null || Math.abs(currentCol - desired) > 2) {
    desired = currentCol;
    view.dispatch({ effects: setDesiredColumn.of(desired) });
  }

  const targetLineNumber = currentLine.number + direction;
  if (targetLineNumber < 1 || targetLineNumber > state.doc.lines) {
    return false;
  }

  const targetLine = state.doc.line(targetLineNumber);
  const padding = getPadding(state, targetLine.length, desired);
  const newPos = targetLine.from + targetLine.length + padding.length;

  const changes = padding.length > 0
    ? { from: targetLine.to, insert: padding }
    : undefined;

  const newSelection = extendSelection
    ? EditorSelection.range(sel.anchor, newPos)
    : EditorSelection.cursor(newPos, undefined, undefined, desired);

  view.dispatch({
    changes,
    selection: newSelection,
    scrollIntoView: true,
    userEvent: 'input.type',
  });

  return true;
}

// Custom commands
const cursorLineUpWithVirtual = (view) => moveVertically(view, -1, false);
const cursorLineDownWithVirtual = (view) => moveVertically(view, 1, false);
const selectLineUpWithVirtual = (view) => moveVertically(view, -1, true);
const selectLineDownWithVirtual = (view) => moveVertically(view, 1, true);

// State effect to set desired column
const setDesiredColumn = StateEffect.define();

/**
 * State field to remember the desired column across movements.
 */
const desiredColumnField = StateField.define({
  create: () => null,
  update(value, tr) {
    for (const e of tr.effects) {
      if (e.is(setDesiredColumn)) return e.value;
    }
    if (tr.selection && !tr.docChanged) {
      const head = tr.selection.main.head;
      return getVisualColumn(tr.startState, head);
    }
    return value;
  },
});

/**
 * ViewPlugin that trims trailing whitespace when cursor leaves a line.
 */
const trimOnLeavePlugin = ViewPlugin.fromClass(class {
  constructor() {
    this.prevLine = null;
  }

  update(update) {
    if (!update.selectionSet && !update.docChanged) return;

    const sel = update.state.selection.main;
    const currentLine = update.state.doc.lineAt(sel.head).number;

    if (this.prevLine !== null && this.prevLine !== currentLine) {
      this.trimTrailingWhitespace(update.view, this.prevLine);
    }
    this.prevLine = currentLine;
  }

  /**
    * Trim trailing whitespace from a specific line.
    * @param {EditorView} view - The editor view
    * @param {number} lineNumber - The line number to trim
    */
  trimTrailingWhitespace(view, lineNumber) {
    // Safety check: ensure the line number is valid
    if (lineNumber < 1 || lineNumber > view.state.doc.lines) {
      return;
    }

    const line = view.state.doc.line(lineNumber);
    const text = line.text;
    const trimmed = text.replace(/\s+$/, '');

    if (trimmed.length !== text.length) {
      view.dispatch({
        changes: {
          from: line.from + trimmed.length,
          to: line.to,
          insert: '',
        },
        userEvent: 'trim.trailing',
      });
    }
  }
});

/**
 * Create the virtual column extension.
 * @returns {Extension[]} The CodeMirror extensions for virtual columns
 */
function virtualColumns() {
  return [
    desiredColumnField,
    trimOnLeavePlugin,
    keymap.of([
      { key: 'ArrowUp', run: cursorLineUpWithVirtual, preventDefault: true },
      { key: 'ArrowDown', run: cursorLineDownWithVirtual, preventDefault: true },
      { key: 'Shift-ArrowUp', run: selectLineUpWithVirtual, preventDefault: true },
      { key: 'Shift-ArrowDown', run: selectLineDownWithVirtual, preventDefault: true },
    ]),
  ];
}

/**
 * Shared CodeMirror 6 Setup
 *
 * Provides a consistent set of extensions for all CodeMirror instances across
 * managers.  Every editor gets:
 *   1) 4-digit zero-padded line numbers
 *   2) Fold gutter + code folding for the active language
 *   3) A per-language background tint (via CSS variable --edit-lang-*)
 *   4) oneDark theme
 *   5) Highlight active line / active line gutter
 *   6) Draw selection, special chars, history, keymaps
 *   7) A Compartment for readonly state that can be toggled at runtime
 *   8) Custom scrollbars (replaces native browser scrollbars)
 *
 * Additionally exposes helpers:
 *   - `setEditorEditable(view, compartment, editable, container?, readonlyClass?)`
 *   - `foldAllInEditor(view)` / `unfoldAllInEditor(view)`
 *   - `foldToLevel(view, level)` - Fold to a specific nesting level
 *   - `unfoldOneLevel(view)` - Expand only the next level of folded regions
 *   - `sortJsonKeys(data)` - Recursively sort JSON object keys
 *   - `formatSortedJson(data, space)` - Format JSON with sorted keys
 *   - `parseAndSortJson(jsonString)` - Parse and normalize JSON with sorted keys
 *   - `compareJsonIgnoringKeyOrder(a, b)` - Compare JSON ignoring key order
 *   - Language-background CSS class constant (`LANG_CLASS_PREFIX`)
 *
 * Usage:
 *   import { buildEditorExtensions, createReadOnlyCompartment, setEditorEditable }
 *     from '../../core/codemirror-setup.js';
 *
 *   const roCompartment = createReadOnlyCompartment();
 *   const extensions = buildEditorExtensions({
 *     language: 'sql',          // 'sql' | 'json' | 'css' | 'markdown' | 'javascript' | 'html' | 'log'
 *     readOnlyCompartment: roCompartment,
 *     readOnly: true,
 *     fontSize: 14,
 *     onUpdate: (update) => { ... },
 *     onEscape: () => { ... },  // Called when Escape is pressed
 *     onSave: () => { ... },    // Called when Ctrl+Enter is pressed
 *   });
 *
 * @module core/codemirror-setup
 */


/** Facet to store the indent string (tab or spaces) for manual insertion */
const indentStringFacet = Facet.define({
  combine: (values) => values[0] || '\t',
});

// ── Whitespace Rendering ─────────────────────────────────────────────────────

class NewlineWidget extends WidgetType {
  toDOM() {
    const span = document.createElement("span");
    span.textContent = "¶";
    span.className = "cm-newline-marker";
    return span;
  }
}

const newlinePlugin = ViewPlugin.fromClass(class {
  decorations = Decoration.none;

  update(update) {
    this.decorations = this.compute(update.view);
  }

  compute(view) {
    const decorations = [];
    for (let i = 1; i <= view.state.doc.lines; i++) {
      const line = view.state.doc.line(i);
      if (i < view.state.doc.lines) { // Only for lines with newlines
        decorations.push(
          Decoration.widget({
            widget: new NewlineWidget(),
            block: false
          }).range(line.to, line.to)
        );
      }
    }
    return Decoration.set(decorations);
  }
}, {
  decorations: v => v.decorations
});

// ── Selection Highlight ─────────────────────────────────────────────────────

/**
 * Create a Compartment for selection highlight state.
 * @returns {Compartment}
 */
function createSelectionHighlightCompartment() {
  return new Compartment();
}

/**
 * Toggle selection highlight on a CodeMirror editor.
 * @param {EditorView} view - The EditorView
 * @param {Compartment} compartment - Selection highlight compartment
 * @param {boolean} enabled - Whether to enable selection highlighting
 */
function toggleSelectionHighlight(view, compartment, enabled) {
  if (!view || !compartment) return;
  view.dispatch({
    effects: compartment.reconfigure(enabled ? highlightSelectionMatches() : []),
  });
  const container = view.dom.parentElement;
  if (container) {
    container.classList.toggle(SELECTION_HIGHLIGHT_CLASS, enabled);
  }
}

// ── Comment Continuation ────────────────────────────────────────────────────

/**
 * Create a Compartment for comment continuation state.
 * @returns {Compartment}
 */
function createCommentContinuationCompartment() {
  return new Compartment();
}

/**
 * Toggle comment continuation on a CodeMirror editor.
 * @param {EditorView} view - The EditorView
 * @param {Compartment} compartment - Comment continuation compartment
 * @param {boolean} enabled - Whether to enable comment continuation
 */
function toggleCommentContinuation(view, compartment, enabled) {
  if (!view || !compartment) return;
  const commentKeymap = enabled ? [
    { key: "Mod-/", run: toggleLineComment },
    { key: "Shift-Mod-a", run: toggleBlockComment },
  ] : [];
  view.dispatch({
    effects: compartment.reconfigure(keymap.of(commentKeymap)),
  });
  const container = view.dom.parentElement;
  if (container) {
    container.classList.toggle(COMMENT_CONTINUATION_CLASS, enabled);
  }
}

// ── Whitespace Rendering ─────────────────────────────────────────────────────

/**
 * Create a Compartment for whitespace rendering state.
 * @returns {Compartment}
 */
function createWhitespaceCompartment() {
  return new Compartment();
}

function createIndentUnitCompartment() {
  return new Compartment();
}

// ── Virtual Columns ──────────────────────────────────────────────────────────

/**
 * Create a Compartment for virtual columns state.
 * @returns {Compartment}
 */
function createVirtualColumnsCompartment() {
  return new Compartment();
}

/**
 * Toggle whitespace rendering on a CodeMirror editor.
 * @param {EditorView} view - The EditorView
 * @param {Compartment} compartment - Whitespace compartment
 * @param {boolean} enabled - Whether to enable whitespace rendering
 */
function toggleWhitespace(view, compartment, enabled) {
  if (!view || !compartment) return;
  view.dispatch({
    effects: compartment.reconfigure(enabled ? [highlightWhitespace(), newlinePlugin] : []),
  });
  const container = view.dom.parentElement;
  if (container) {
    container.classList.toggle('lithium-cm-show-whitespace', enabled);
  }
  // Force a redraw to ensure newline markers appear immediately
  if (enabled) {
    // Use requestAnimationFrame to ensure the compartment change is processed first
    requestAnimationFrame(() => {
      // Dispatch an empty transaction to trigger plugin updates
      view.dispatch({ changes: [] });
    });
  }
}

/**
 * Toggle virtual columns on a CodeMirror editor.
 * @param {EditorView} view - The EditorView
 * @param {Compartment} compartment - Virtual columns compartment
 * @param {boolean} enabled - Whether to enable virtual columns
 */
function toggleVirtualColumns(view, compartment, enabled) {
  if (!view || !compartment) return;
  view.dispatch({
    effects: compartment.reconfigure(enabled ? virtualColumns() : []),
  });
  const container = view.dom.parentElement;
  if (container) {
    container.classList.toggle('lithium-cm-virtual-columns', enabled);
  }
}


// ── Constants ─────────────────────────────────────────────────────────────

/** CSS class prefix applied to the EditorView wrapper for language-based theming */
const LANG_CLASS_PREFIX = 'lithium-cm-lang-';

/** CSS class applied to the EditorView wrapper when the editor is in edit mode */
const EDIT_MODE_CLASS = 'lithium-cm-editable';

/** CSS class applied to the EditorView wrapper when the editor is readonly */
const READONLY_CLASS = 'lithium-cm-readonly';

/** CSS class applied to the EditorView wrapper when word wrap is enabled */
const WORD_WRAP_CLASS = 'lithium-cm-wordwrap';

/** CSS class applied to the EditorView wrapper when bracket matching is enabled */
const BRACKET_MATCH_CLASS = 'lithium-cm-bracketmatch';

/** CSS class applied to the EditorView wrapper when selection highlighting is enabled */
const SELECTION_HIGHLIGHT_CLASS = 'lithium-cm-selection-highlight';

/** CSS class applied to the EditorView wrapper when comment continuation is enabled */
const COMMENT_CONTINUATION_CLASS = 'lithium-cm-comment-continuation';

/** CSS class applied to the EditorView wrapper when whitespace rendering is enabled */
const WHITESPACE_CLASS = 'lithium-cm-show-whitespace';





// ── Language Resolution ───────────────────────────────────────────────────

const LANGUAGE_MAP = {
  sql: () => sql(),
  json: () => json(),
  css: () => css(),
  markdown: () => markdown(),
  javascript: () => javascript(),
  html: () => html(),
  log: () => sql(),       // Logs use SQL highlighting for query snippets
};

/**
 * Get the CodeMirror language extension for a language string.
 * @param {string} lang
 * @returns {Extension}
 */
function getLanguageExtension(lang) {
  const factory = LANGUAGE_MAP[lang] || LANGUAGE_MAP.sql;
  return factory();
}

// ── Compartment Factory ───────────────────────────────────────────────────

/**
 * Create a fresh `Compartment` for toggling readonly state.
 * @returns {Compartment}
 */
function createReadOnlyCompartment() {
  return new Compartment();
}

// ── Extension Builder ─────────────────────────────────────────────────────

/**
 * Build the standard set of CodeMirror extensions.
 *
 * @param {Object} options
 * @param {string}       options.language             - Language key (sql, json, css, markdown, javascript, html, log)
 * @param {Compartment}  options.readOnlyCompartment  - Compartment for toggling readonly
 * @param {boolean}      [options.readOnly=true]      - Initial readonly state
 * @param {number}       [options.fontSize=14]        - Font size in px
 * @param {string}       [options.fontFamily]         - CSS font-family string
 * @param {Function}     [options.onUpdate]           - EditorView.updateListener callback
 * @param {boolean}      [options.lineNumbersEnabled=true] - Show line numbers
 * @param {Function}     [options.onEscape]           - Callback when Escape is pressed
 * @param {Function}     [options.onSave]             - Callback when Ctrl+Enter is pressed
 * @param {Compartment}  [options.bracketMatchCompartment] - Compartment for bracket matching toggle
 * @param {boolean}      [options.bracketMatch=true]  - Initial bracket matching state
 * @param {Compartment}  [options.wordWrapCompartment] - Compartment for word wrap toggle
 * @param {boolean}      [options.wordWrap=false]     - Initial word wrap state
 * @param {Compartment}  [options.whitespaceCompartment] - Compartment for whitespace rendering toggle
 * @param {boolean}      [options.showWhitespace=false] - Show whitespace characters (spaces as dots, tabs as arrows)
 * @param {Compartment}  [options.virtualColumnsCompartment] - Compartment for virtual columns toggle
 * @param {boolean}      [options.virtualColumns=false] - Enable virtual column cursor behavior
 * @param {Compartment}  [options.indentUnitCompartment] - Compartment for indent string (tab/spaces)
 * @param {string}       [options.initialIndentString='\t'] - Initial indent string (tab or spaces)
 * @param {number}       [options.indentUnitValue=1] - Initial indent unit (number for language auto-indent)
 * @returns {Extension[]}
 */
function buildEditorExtensions(options = {}) {
  const {
    language = 'sql',
    readOnlyCompartment,
    readOnly = true,
    fontSize = 14,
    fontFamily = '"Vanadium Mono", var(--font-mono, monospace)',
    onUpdate,
    lineNumbersEnabled = true,
    onEscape,
    onSave,
    bracketMatchCompartment,
    bracketMatch = true,
    wordWrapCompartment,
    wordWrap = false,
    selectionHighlightCompartment,
    selectionHighlight = false,
    commentContinuationCompartment,
    commentContinuation = false,
    whitespaceCompartment,
    showWhitespace = false,
    virtualColumnsCompartment,
    virtualColumns: enableVirtualColumns = true,
    indentUnitCompartment,
    initialIndentString = '\t',
  } = options;

   const langCssVar = `var(--edit-lang-${language}, var(--edit-lang-default))`;

   const extensions = [];

   // 1) Line numbers with 4-digit zero-padded format
   if (lineNumbersEnabled) {
     extensions.push(
       lineNumbers({ formatNumber: (n) => String(n).padStart(4, '0') }),
     );
   }

   // 2) Code folding — gutter markers + keyboard shortcuts
   extensions.push(
     foldGutter({
       openText: '\u25BC',   // ▼ (down - shows when expanded to collapse)
       closedText: '\u25B6', // ▶ (right - shows when collapsed to expand)
     }),
     keymap.of(foldKeymap),
   );

   // 3) Visual chrome
    extensions.push(
      highlightActiveLineGutter(),
      highlightSpecialChars(),
      drawSelection(),
      highlightActiveLine(),
      indentOnInput(),
    );

    // 4) Transaction filter to replace tab characters with indent string when indent unit is spaces
    extensions.push(
      EditorState.transactionFilter.of((tr) => {
        if (!tr.docChanged) return tr;
        const indentString = tr.startState.facet(indentStringFacet);
        // If indent string is tab, allow tab characters
        if (indentString === '\t') return tr;
        
        // Check if any inserted text contains tab characters
        let hasTab = false;
        tr.changes.iterChanges((fromA, toA, fromB, toB, inserted) => {
          const text = inserted.sliceString(0, inserted.length, '\n');
          if (text.includes('\t')) hasTab = true;
        });
        if (!hasTab) return tr;
        
        // Build new changes with tabs replaced by indent string
        const newChanges = [];
        tr.changes.iterChanges((fromA, toA, fromB, toB, inserted) => {
          const text = inserted.sliceString(0, inserted.length, '\n');
          const replaced = text.replace(/\t/g, indentString);
          newChanges.push({ from: fromA, to: toA, insert: replaced });
        });
        
        // Return new transaction with replaced changes, preserving selection and effects
        return [tr.startState.update({
          changes: newChanges,
          selection: tr.selection,
          effects: tr.effects,
          annotations: tr.annotations,
        })];
      })
    );

   // 4) History + keymaps
   const customKeymap = [];

   // Add Escape key handler for canceling edit mode
   if (onEscape) {
     customKeymap.push({
       key: 'Escape',
       run: () => {
         onEscape();
         return true; // Prevent default CodeMirror behavior
       },
     });
   }

   // Add Ctrl+Enter handler for saving
   if (onSave) {
     customKeymap.push({
       key: 'Ctrl-Enter',
       run: () => {
         onSave();
         return true; // Prevent default CodeMirror behavior
       },
     });
   }

   // Handle Tab key to insert current indent string (tab or spaces)
   customKeymap.push({
     key: 'Tab',
     run: (view) => {
       if (view.state.readOnly) {
         // If readonly, prevent default (don't tab out)
         return true;
       }
       // If editable, insert the current indent string
       const currentIndent = view.state.facet(indentStringFacet);
       const changes = [];
       const { state } = view;

       state.selection.ranges.forEach(range => {
         if (range.empty) {
           // Single cursor: insert indent string
           changes.push({ from: range.from, to: range.to, insert: currentIndent });
         } else {
           // Selection: indent the selected lines
           const fromLine = state.doc.lineAt(range.from).number;
           const toLine = state.doc.lineAt(range.to).number;
           for (let lineNo = fromLine; lineNo <= toLine; lineNo++) {
             const line = state.doc.line(lineNo);
             changes.push({ from: line.from, to: line.from, insert: currentIndent });
           }
         }
       });

       view.dispatch({ changes });
       return true; // Prevent default behavior
     },
   });

   // Handle Enter key to insert newline with current indent string
   customKeymap.push({
     key: 'Enter',
     run: (view) => {
       if (view.state.readOnly) {
         // If readonly, allow default behavior (might be needed for form submission)
         return false;
       }
       // If editable, insert newline + current indent string
       const currentIndent = view.state.facet(indentStringFacet);
       const changes = [];
       const { state } = view;

       state.selection.ranges.forEach(range => {
         // Insert newline + current indent string
         changes.push({ from: range.from, to: range.to, insert: '\n' + currentIndent });
       });

       view.dispatch({ changes });
       return true; // Prevent default behavior
     },
   });

   extensions.push(
     history(),
     keymap.of([...customKeymap, ...defaultKeymap, ...historyKeymap]),
   );

   // 5) Language
   extensions.push(getLanguageExtension(language));

   // 6) Theme (oneDark as base, then our per-language overrides)
   extensions.push(oneDark);

   extensions.push(
     EditorView.theme({
       '&': {
         height: '100%',
         fontSize: `${fontSize}px`,
         background: langCssVar,
       },
       '.cm-scroller': {
         overflow: 'auto',
         fontFamily,
       },
       '.cm-content': {
         fontFamily,
         caretColor: 'var(--accent-primary, #5C6BC0)',
       },
       '.cm-gutters': {
         background: 'var(--edit-gutter-bg, var(--bg-tertiary))',
         borderRight: '1px solid var(--border-color)',
         fontFamily,
         transition: 'background var(--transition-duration, 350ms) ease',
       },
       '.cm-lineNumbers .cm-gutterElement': {
         minWidth: '3.5ch',
         paddingLeft: '4px',
         paddingRight: '8px',
         color: 'var(--text-secondary)',
       },
       // Fold gutter markers
       '.cm-foldGutter .cm-gutterElement:hover': {
         color: 'var(--text-primary)',
       },
     }),
   );

   // 6.5) Indent string (tab or spaces) - configurable
   if (indentUnitCompartment) {
     extensions.push(indentUnitCompartment.of(indentStringFacet.of(initialIndentString)));
   } else {
     extensions.push(indentStringFacet.of(initialIndentString));
   }

   // 7) Readonly compartment
   if (readOnlyCompartment) {
     extensions.push(
       readOnlyCompartment.of(EditorState.readOnly.of(readOnly)),
     );
   }

   // 8) Bracket matching compartment
   if (bracketMatchCompartment) {
     extensions.push(
       bracketMatchCompartment.of(bracketMatch ? bracketMatching() : []),
     );
   }

   // 9) Word wrap compartment
   if (wordWrapCompartment) {
     extensions.push(
       wordWrapCompartment.of(wordWrap ? EditorView.lineWrapping : []),
     );
   }

   // 10) Whitespace rendering compartment
   if (whitespaceCompartment) {
     extensions.push(
       whitespaceCompartment.of(showWhitespace ? [highlightWhitespace(), newlinePlugin] : []),
     );
   }

   // 11) Virtual columns compartment
   if (virtualColumnsCompartment) {
     extensions.push(
       virtualColumnsCompartment.of(enableVirtualColumns ? virtualColumns() : []),
     );
   }

   // 13) Selection highlight compartment
   if (selectionHighlightCompartment) {
     extensions.push(
       selectionHighlightCompartment.of(selectionHighlight ? highlightSelectionMatches() : []),
     );
   }

   // 14) Comment continuation compartment
   if (commentContinuationCompartment) {
     const commentKeymap = commentContinuation ? [
       { key: "Mod-/", run: toggleLineComment },
       { key: "Shift-Mod-a", run: toggleBlockComment },
     ] : [];
     extensions.push(
       commentContinuationCompartment.of(keymap.of(commentKeymap)),
     );
   }

   // 13) Update listener (for dirty tracking, etc.)
   if (onUpdate) {
     extensions.push(EditorView.updateListener.of(onUpdate));
   }

   // 14) Virtual columns (optional)
   if (enableVirtualColumns) {
     extensions.push(...virtualColumns());
   }

   // 15) Custom scrollbars
   extensions.push(customScrollbars());

    return extensions;
  }

  // ── Editor State Helpers ──────────────────────────────────────────────────

/**
 * Toggle a CodeMirror editor between readonly and editable.
 *
 * Handles:
 *   - Compartment reconfiguration (CM6 readOnly state)
 *   - contentEditable toggle on .cm-content DOM element
 *   - CSS class for visual styling (gutter color change)
 *
 * @param {EditorView}  view          - The CM6 EditorView
 * @param {Compartment} compartment   - The readonly compartment
 * @param {boolean}     editable      - true = editable, false = readonly
 * @param {HTMLElement}  [container]   - Optional container element for CSS class toggling
 */
function setEditorEditable(view, compartment, editable, container) {
  if (!view || !compartment) return;

  // 1) Reconfigure the readonly compartment
  view.dispatch({
    effects: compartment.reconfigure(EditorState.readOnly.of(!editable)),
  });

  // 2) Toggle contentEditable on .cm-content
  const contentEl = view.dom.querySelector('.cm-content');
  if (contentEl) contentEl.contentEditable = editable ? 'true' : 'false';

  // 3) Toggle CSS classes on the container (or the editor DOM)
  const target = container || view.dom.parentElement;
  if (target) {
    target.classList.toggle(EDIT_MODE_CLASS, editable);
    target.classList.toggle(READONLY_CLASS, !editable);
  }
}

// ── Fold All / Unfold All ─────────────────────────────────────────────────

/**
 * Collapse all foldable regions recursively (every level).
 * Walks the syntax tree and folds every foldable range.
 * @param {EditorView} view
 */
function foldAllInEditor(view) {
  if (!view) return;

  const { state } = view;
  const effects = [];

  syntaxTree(state).iterate({
    enter(node) {
      const fold = foldable(state, node.from, node.to);
      if (fold) {
        effects.push(foldEffect.of(fold));
      }
    },
  });

  if (effects.length) {
    view.dispatch({ effects });
  }
}

/**
 * Expand all folded regions.
 * Uses the built-in unfoldAll which works correctly.
 * @param {EditorView} view
 */
function unfoldAllInEditor(view) {
  if (!view) return;
  unfoldAll(view);
}

// ── Level-Based Folding ───────────────────────────────────────────────────

/**
 * Get the foldable regions in the document with their nesting depth.
 * @param {EditorView} view
 * @returns {Array<{from: number, to: number, depth: number}>}
 */
function getFoldableRegions(view) {
  if (!view) return [];

  const regions = [];
  const { state } = view;
  const tree = syntaxTree(state);

  tree.iterate({
    enter(node) {
      // Check if this node type supports folding
      const foldRange = view.state.languageDataAt('foldRange', node.from)[0];

      if (foldRange) {
        const range = foldRange(state, node.from, node.to);
        if (range) {
          regions.push({
            from: range.from,
            to: range.to,
            depth: 0, // Will be calculated below
          });
        }
      }
    },
  });

  // Calculate nesting depth for each region
  regions.forEach((region) => {
    region.depth = regions.filter(
      (r) => r.from < region.from && r.to > region.to
    ).length;
  });

  return regions;
}

/**
 * Fold the editor to show only the specified nesting level.
 * Level 0 = fold everything, Level 1 = show first level, etc.
 * @param {EditorView} view
 * @param {number} targetLevel - The level to fold to (0 = fold all, 1 = show level 1, etc.)
 */
function foldToLevel(view, targetLevel = 1) {
  if (!view) return;

  // First unfold everything to get accurate depth calculation
  unfoldAll(view);

  const regions = getFoldableRegions(view);
  if (regions.length === 0) return;

  // Fold regions deeper than targetLevel
  regions.forEach((region) => {
    if (region.depth >= targetLevel) {
      foldCode(view, region.from);
    }
  });
}

/**
 * Fold all regions (same as foldAllInEditor but with clearer semantics).
 * This collapses everything to the root level.
 * @param {EditorView} view
 */
function foldAllRegions(view) {
  if (!view) return;
  foldAll(view);
}

/**
 * Unfold all regions to show the full document.
 * @param {EditorView} view
 */
function unfoldAllRegions(view) {
  if (!view) return;
  unfoldAll(view);
}

/**
 * Expand only the next level of folded regions.
 * Useful for progressive disclosure - shows one more level of detail.
 * @param {EditorView} view
 */
function unfoldOneLevel(view) {
  if (!view) return;

  const { state } = view;
  const foldedRanges = [];

  // Find all currently folded regions
  state.facet(EditorState.folded).forEach((range) => {
    foldedRanges.push({ from: range.from, to: range.to });
  });

  if (foldedRanges.length === 0) return;

  // Unfold only the outermost folded regions (those not contained by other folds)
  const outermostFolds = foldedRanges.filter((range) => {
    return !foldedRanges.some(
      (other) => other.from < range.from && other.to > range.to
    );
  });

  // Unfold these outermost regions
  outermostFolds.forEach((range) => {
    unfoldCode(view, range.from);
  });
}

// ── Programmatic Content (No History) ────────────────────────────────────

/**
 * Replace the full content of a CodeMirror editor WITHOUT adding to
 * the undo history.  Use this for programmatic content loads (row
 * selection, cancel restore) so that undo only covers user edits.
 *
 * @param {EditorView} view     - The CodeMirror EditorView
 * @param {string}     content  - New document content
 */
function setEditorContentNoHistory(view, content) {
  if (!view) return;
  view.dispatch({
    changes: { from: 0, to: view.state.doc.length, insert: content },
    annotations: Transaction.addToHistory.of(false),
  });
}

// ── Undo/Redo Button State ───────────────────────────────────────────────

/**
 * Update the enabled/disabled state of undo/redo toolbar buttons based on
 * the active editor's history depth and whether the manager is in edit mode.
 *
 * This is the shared implementation used by all managers with CodeMirror
 * editors and undo/redo toolbar buttons.  Call it from:
 *   - Each editor's `onUpdate` listener (after every transaction)
 *   - `editHelper.onAfterEditModeChange` (entering/exiting edit mode)
 *   - Tab switching (the active editor may have a different history)
 *
 * @param {Object} options
 * @param {HTMLElement}   options.undoBtn   - The undo button element
 * @param {HTMLElement}   options.redoBtn   - The redo button element
 * @param {EditorView|null} options.view    - The currently active CodeMirror EditorView (null = no editor)
 * @param {boolean}       options.isEditing - Whether the manager is in edit mode
 */
function updateUndoRedoButtons({ undoBtn, redoBtn, view, isEditing }) {
  if (!undoBtn || !redoBtn) return;

  if (!isEditing || !view) {
    undoBtn.disabled = true;
    redoBtn.disabled = true;
    return;
  }

  undoBtn.disabled = undoDepth(view.state) === 0;
  redoBtn.disabled = redoDepth(view.state) === 0;
}

// ── JSON Sorting Utilities ────────────────────────────────────────────────

/**
 * Recursively sort the keys of JSON objects while preserving array order.
 * This ensures consistent JSON formatting for comparisons and server submission.
 *
 * @param {*} data - The data to sort (object, array, or primitive)
 * @returns {*} - The sorted data
 */
function sortJsonKeys(data) {
  // Handle null
  if (data === null) return null;

  // Handle arrays - preserve order, but sort keys of any objects within
  if (Array.isArray(data)) {
    return data.map((item) => sortJsonKeys(item));
  }

  // Handle objects - sort keys alphabetically
  if (typeof data === 'object' && data !== null) {
    const sorted = {};
    Object.keys(data)
      .sort((a, b) => a.localeCompare(b))
      .forEach((key) => {
        sorted[key] = sortJsonKeys(data[key]);
      });
    return sorted;
  }

  // Primitives (string, number, boolean) - return as-is
  return data;
}

/**
 * Format JSON with sorted keys for consistent display/output.
 * Use this when loading JSON into CodeMirror or sending to the server.
 *
 * @param {*} data - The data to format
 * @param {number} [space=2] - Number of spaces for indentation
 * @returns {string} - The formatted JSON string with sorted keys
 */
function formatSortedJson(data, space = 2) {
  const sorted = sortJsonKeys(data);
  return JSON.stringify(sorted, null, space);
}

/**
 * Parse JSON and return it with sorted keys.
 * Useful for normalizing JSON from various sources before comparison.
 *
 * @param {string} jsonString - The JSON string to parse and sort
 * @param {number} [space=2] - Number of spaces for indentation (if returning as string)
 * @returns {string|null} - The sorted JSON string, or null if parsing fails
 */
function parseAndSortJson(jsonString, space = 2) {
  try {
    const parsed = JSON.parse(jsonString);
    return formatSortedJson(parsed, space);
  } catch (e) {
    console.error('[codemirror-setup] Failed to parse JSON:', e);
    return null;
  }
}

/**
 * Compare two JSON strings for equality, ignoring key order differences.
 * Useful for dirty checking - detects if content has meaningfully changed.
 *
 * @param {string} jsonA - First JSON string
 * @param {string} jsonB - Second JSON string
 * @returns {boolean} - True if the JSON objects are equivalent (ignoring key order)
 */
function compareJsonIgnoringKeyOrder(jsonA, jsonB) {
  const sortedA = parseAndSortJson(jsonA, 0);
  const sortedB = parseAndSortJson(jsonB, 0);

  if (sortedA === null || sortedB === null) {
    // If parsing fails, fall back to string comparison
    return jsonA === jsonB;
  }

  return sortedA === sortedB;
}

// ── Word Wrap Compartment ─────────────────────────────────────────────

/**
 * Create a Compartment for word wrap state.
 * @returns {Compartment}
 */
function createWordWrapCompartment() {
  return new Compartment();
}

/**
 * Toggle word wrap on a CodeMirror editor.
 * @param {EditorView} view - The EditorView
 * @param {Compartment} compartment - Word wrap compartment
 * @param {boolean} enabled - Whether to enable word wrap
 */
function toggleWordWrap(view, compartment, enabled) {
  if (!view || !compartment) return;
  view.dispatch({
    effects: compartment.reconfigure(enabled ? EditorView.lineWrapping : []),
  });
  const container = view.dom.parentElement;
  if (container) {
    container.classList.toggle(WORD_WRAP_CLASS, enabled);
  }
}

// ── Bracket Matching Compartment ─────────────────────────────────────────

/**
 * Create a Compartment for bracket matching state.
 * @returns {Compartment}
 */
function createBracketMatchCompartment() {
  return new Compartment();
}

/**
 * Toggle bracket matching on a CodeMirror editor.
 * @param {EditorView} view - The EditorView
 * @param {Compartment} compartment - Bracket match compartment
 * @param {boolean} enabled - Whether to enable bracket matching
 */
function toggleBracketMatch(view, compartment, enabled) {
  if (!view || !compartment) return;
  view.dispatch({
    effects: compartment.reconfigure(enabled ? bracketMatching() : []),
  });
  const container = view.dom.parentElement;
  if (container) {
    container.classList.toggle(BRACKET_MATCH_CLASS, enabled);
  }
}

export { BRACKET_MATCH_CLASS, COMMENT_CONTINUATION_CLASS, EDIT_MODE_CLASS, LANG_CLASS_PREFIX, READONLY_CLASS, SELECTION_HIGHLIGHT_CLASS, WHITESPACE_CLASS, WORD_WRAP_CLASS, buildEditorExtensions, compareJsonIgnoringKeyOrder, createBracketMatchCompartment, createCommentContinuationCompartment, createIndentUnitCompartment, createReadOnlyCompartment, createSelectionHighlightCompartment, createVirtualColumnsCompartment, createWhitespaceCompartment, createWordWrapCompartment, foldAllInEditor, foldAllRegions, foldToLevel, formatSortedJson, indentStringFacet, parseAndSortJson, setEditorContentNoHistory, setEditorEditable, sortJsonKeys, toggleBracketMatch, toggleCommentContinuation, toggleSelectionHighlight, toggleVirtualColumns, toggleWhitespace, toggleWordWrap, unfoldAllInEditor, unfoldAllRegions, unfoldOneLevel, updateUndoRedoButtons };
