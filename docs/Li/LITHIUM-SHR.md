# Lithium Shared Components (LITHIUM-SHR.md)

This document describes the reusable components available in the `src/shared/` directory. These components are designed to be used across multiple managers and features throughout the Lithium application.

## Available Components

### TimezonePicker

**Location**: `src/shared/timezone-picker.js`

A resizable, searchable timezone dropdown picker with grouped display.

**Features**:
- Resizable popup with custom dimensions
- Search/filter functionality
- Grouped timezone display (Current, Abbreviated, Major, Regional)
- Real-time time display for each timezone
- Proper popup management integration

**Usage**:
```javascript
import { TimezonePicker } from '../../shared/timezone-picker.js';

const picker = new TimezonePicker(container, timezones, onSelect, onPositionChange);
picker.init(triggerButton, inputElement);
```

**Used by**: Date Formats page (Profile Manager)

### FlatpickrPicker

**Location**: `src/shared/flatpickr-picker.js`

A reusable Flatpickr date/time picker with popup positioning and animations.

**Features**:
- Header-style popup positioning
- Scale animations matching Lithium popup system
- Outside click and ESC key handling
- Integration with `close-all-popups` event
- Proper cleanup and state management

**Usage**:
```javascript
import { FlatpickrPicker } from '../../shared/flatpickr-picker.js';

const picker = new FlatpickrPicker({
  onChange: (date, formattedValue) => { /* handle change */ },
});
picker.init(triggerButton, inputElement);
```

**Used by**: Date Formats page (Profile Manager)

### LuxonTokenTable

**Location**: `src/shared/luxon-token-table.js`

A comprehensive reference table for Luxon date/time format tokens.

**Features**:
- Complete Luxon format token reference
- Live examples using sample and current datetime
- Grouped by category (Timezones, Years, Months, etc.)
- Integrated with LithiumTable for navigation and search
- Auto-updates current time examples

**Usage**:
```javascript
import { LuxonTokenTable } from '../../shared/luxon-token-table.js';

const tokenTable = new LuxonTokenTable({
  container: tableContainer,
  navigatorContainer: navContainer,
  sampleDateTime: DateTime.fromISO('2020-01-01T14:03:02'),
  onTokenSelected: () => closeAllPopups(),
});
await tokenTable.init();
```

**Used by**: Date Formats page (Profile Manager)

## Design Guidelines

### Component Structure

All shared components should follow this pattern:

1. **ES6 Class Export**: Export a class as the default export
2. **Constructor Options**: Accept configuration options in constructor
3. **Init Method**: Separate initialization method for DOM-dependent setup
4. **Cleanup**: Provide proper destroy/cleanup methods
5. **Event Integration**: Integrate with Lithium's event system (close-all-popups, etc.)

### Naming Conventions

- Component classes: `PascalCase` (e.g., `TimezonePicker`)
- File names: `kebab-case.js` (e.g., `timezone-picker.js`)
- Event handlers: `camelCase` with descriptive names

### Dependencies

- Use relative imports for Lithium-specific modules
- Import external libraries directly
- Avoid circular dependencies
- Document external dependencies in component headers

## Adding New Shared Components

When adding new reusable components:

1. Place the component file in `src/shared/`
2. Add documentation to this file (LITHIUM-SHR.md)
3. Update LITHIUM-INS.md if the component affects coding standards
4. Test the component in multiple contexts
5. Ensure proper cleanup and memory management

## Maintenance

- Keep this document updated when adding/removing components
- Review components periodically for potential consolidation
- Consider moving components to manager-specific folders if they become manager-specific
- Ensure components remain compatible with the latest Lithium standards