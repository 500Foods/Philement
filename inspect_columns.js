// Run this in the browser console when the Column Manager is open
// to inspect the resolved column definitions

const manager = document.querySelector('.col-manager-popup');
if (!manager) {
  console.log('Column Manager popup not found');
} else {
  // Find the LithiumTable instance
  const tableContainer = manager.querySelector('.lithium-table-container');
  if (tableContainer && tableContainer._lithiumTable) {
    const colTable = tableContainer._lithiumTable;
    console.log('=== Column Manager Table Columns ===');
    colTable.table.getColumns().forEach(col => {
      const def = col.getDefinition();
      console.log(`\nColumn: ${def.field}`);
      console.log('  coltype:', def._originalColtype || 'N/A');
      console.log('  editor:', def.editor);
      console.log('  editorParams:', JSON.stringify(def.editorParams));
      console.log('  formatter:', def.formatter);
    });
  } else {
    console.log('LithiumTable instance not found');
    
    // Alternative: check Tabulator directly
    const tabulatorEl = manager.querySelector('.tabulator');
    if (tabulatorEl && tabulatorEl.tabulator) {
      console.log('=== Raw Tabulator Columns ===');
      tabulatorEl.tabulator.getColumns().forEach(col => {
        const def = col.getDefinition();
        console.log(`${def.field}: editor=${def.editor}, formatter=${def.formatter}`);
      });
    }
  }
}
