/**
 * Column Manager - Column Definition Builder Module
 *
 * Builds column definitions and lookup maps for the column manager table.
 *
 * @module tables/column-manager/cm-columns
 */

/**
 * Build the column definition for the column manager table.
 * @param {Object} cm - ColumnManager instance
 * @returns {Object} Column definition
 */
export function buildColumnManagerDefinition(cm) {
  // Build lookup maps for the lookupFixed coltype columns
  const formatLookup = buildFormatLookup();
  const summaryLookup = buildSummaryLookup();
  const alignmentLookup = buildAlignmentLookup();

  const isManual = cm.orderingMode === 'manual';

  const columns = {};

  // --- Frozen left columns ---

  // Drag handle column (Manual mode only)
  if (isManual) {
    columns.drag_handle = {
      field: 'drag_handle',
      display: '',
      coltype: 'string',
      visible: true,
      sort: false,
      filter: false,
      editable: false,
      frozen: true,
      resizable: false,
      width: 16,
      minWidth: 16,
      maxWidth: 16,
      hozAlign: 'center',
      rowHandle: true,
      formatter: function() {
        return '<i class="fa-duotone fa-thin fa-grip-dots-vertical"></i>';
      },
    };
  }

  // Visible column
  columns.visible = {
    field: 'visible',
    display: 'Vis',
    coltype: 'boolean',
    visible: true,
    sort: false,
    filter: false,
    editable: true,
    width: 46,
    hozAlign: 'center',
    headerSort: false,
    formatter: 'tickCross',
    formatterParams: {
      allowEmpty: false,
      allowTruthy: true,
    },
    editor: 'tickCross',
  };

  // Order column (both modes, editable only in Auto mode)
  columns.order = {
    field: 'order',
    display: '#',
    coltype: 'integer',
    visible: true,
    sort: false,
    filter: false,
    editable: !isManual,
    frozen: true,
    width: 42,
    hozAlign: 'center',
    headerSort: false,
    overrides: !isManual ? {
      editor: 'number',
      editorParams: {
        min: 1,
        max: 999,
        step: 1,
      },
    } : undefined,
  };

  // --- Data columns ---

  columns.field_name = {
    field: 'field_name',
    display: 'Field Name',
    coltype: 'string',
    visible: true,
    sort: !isManual,
    filter: true,
    editable: false,
    minWidth: 120,
    hozAlign: 'left',
    headerSort: !isManual,
  };

  columns.column_name = {
    field: 'column_name',
    display: 'Column Name',
    coltype: 'string',
    visible: true,
    sort: !isManual,
    filter: true,
    editable: true,
    minWidth: 150,
    hozAlign: 'left',
    headerSort: !isManual,
  };

  columns.editable = {
    field: 'editable',
    display: 'Edit',
    coltype: 'boolean',
    visible: true,
    sort: !isManual,
    filter: true,
    editable: true,
    width: 50,
    hozAlign: 'center',
    headerSort: false,
    formatter: 'tickCross',
    formatterParams: {
      allowEmpty: false,
      allowTruthy: true,
    },
    editor: 'tickCross',
  };

  columns.format = {
    field: 'format',
    display: 'Format',
    coltype: 'lookupFixed',
    visible: true,
    sort: !isManual,
    filter: true,
    editable: true,
    width: 120,
    hozAlign: 'left',
    headerSort: !isManual,
    overrides: {
      editor: 'select',
      editorParams: {
        values: formatLookup,
      },
      formatter: 'lookup',
      formatterParams: {
        lookup: formatLookup,
      },
    },
  };

  columns.summary = {
    field: 'summary',
    display: 'Summary',
    coltype: 'lookupFixed',
    visible: true,
    sort: !isManual,
    filter: true,
    editable: true,
    width: 100,
    hozAlign: 'left',
    headerSort: !isManual,
    overrides: {
      editor: 'select',
      editorParams: {
        values: summaryLookup,
      },
      formatter: 'lookup',
      formatterParams: {
        lookup: summaryLookup,
      },
    },
  };

  columns.alignment = {
    field: 'alignment',
    display: 'Alignment',
    coltype: 'lookupFixed',
    visible: true,
    sort: !isManual,
    filter: true,
    editable: true,
    width: 90,
    hozAlign: 'left',
    headerSort: !isManual,
    overrides: {
      editor: 'select',
      editorParams: {
        values: alignmentLookup,
      },
      formatter: 'lookup',
      formatterParams: {
        lookup: alignmentLookup,
      },
    },
  };

  columns.category = {
    field: 'category',
    display: 'Category',
    coltype: 'stringList',
    visible: true,
    sort: !isManual,
    filter: true,
    editable: true,
    minWidth: 100,
    hozAlign: 'left',
    headerSort: !isManual,
  };

  columns.width = {
    field: 'width',
    display: 'Width',
    coltype: 'integer',
    visible: true,
    sort: !isManual,
    filter: false,
    editable: true,
    width: 80,
    hozAlign: 'right',
    headerSort: false,
    overrides: {
      editor: 'number',
      editorParams: {
        min: 20,
        max: 1000,
      },
    },
  };

  return {
    title: 'Column Manager',
    readonly: false,
    layout: 'fitColumns',
    rowHeight: 32,
    movableRows: isManual,
    groupBy: 'category',
    groupStartOpen: true,
    groupToggleElement: 'header',
    columns,
  };
}

/**
 * Build lookup map for format column
 * @returns {Object} Format lookup
 */
function buildFormatLookup() {
  return {
    string: 'Text',
    integer: 'Integer',
    decimal: 'Decimal',
    index: 'Index',
    currency: 'Currency',
    percent: 'Percent',
    boolean: 'Boolean',
    booleanCheckbox: 'Checkbox',
    booleanIcon: 'Icon Bool',
    booleanSwitch: 'Switch',
    date: 'Date',
    datetime: 'DateTime',
    datetimeUTC: 'UTC DateTime',
    time: 'Time',
    duration: 'Duration',
    lookup: 'Lookup',
    lookupFixed: 'Fixed Lookup',
    lookupIcon: 'Icon Lookup',
    lookupIconList: 'Icon List',
    lookupIconText: 'Icon Text',
    stringList: 'String List',
    tags: 'Tags',
    color: 'Color',
    colorPicker: 'Color Picker',
    email: 'Email',
    url: 'URL',
    phone: 'Phone',
    ipAddress: 'IP Address',
    uuid: 'UUID',
    json: 'JSON',
    html: 'HTML',
    text: 'Text Area',
    password: 'Password',
    rating: 'Rating',
    star: 'Stars',
    progress: 'Progress',
    fileSize: 'File Size',
    image: 'Image',
    imageAvatar: 'Avatar',
    emoji: 'Emoji',
    slug: 'Slug',
    version: 'Version',
    rownum: 'Row Num',
  };
}

/**
 * Build lookup map for summary column
 * @returns {Object} Summary lookup
 */
function buildSummaryLookup() {
  return {
    none: 'None',
    count: 'Count',
    sum: 'Sum',
    avg: 'Average',
    min: 'Minimum',
    max: 'Maximum',
    unique: 'Unique',
  };
}

/**
 * Build lookup map for alignment column
 * @returns {Object} Alignment lookup
 */
function buildAlignmentLookup() {
  return {
    left: 'Left',
    center: 'Center',
    right: 'Right',
  };
}
