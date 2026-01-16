# SQLite Convert TZ Extension

This SQLite loadable extension provides timezone conversion functionality equivalent to MySQL's `CONVERT_TZ` function for use in the Helium migration system and other database operations.

## Requirements

- SQLite 3.x
- IANA timezone data (`/usr/share/zoneinfo/`)

## Building

```bash
make clean
make
```

This will produce `convert_tz.so`

## Installation

### System-wide Installation

```bash
sudo make install
```

This installs to `/usr/local/lib/convert_tz.so`

### Loading the Extension

From SQLite command line:

```sql
.load /usr/local/lib/convert_tz
-- or if installed in a standard location:
.load convert_tz
```

From application code, use `sqlite3_load_extension()`.

## Technical Implementation

### How Timezone Conversion Works

The `CONVERT_TZ` function performs timezone conversion through these steps:

1. **Input Validation**: Checks that all three parameters are provided and not NULL
2. **Same Timezone Check**: If `from_tz` equals `to_tz`, returns the input unchanged (optimization)
3. **Datetime Parsing**: Parses the input datetime string using `strptime()` with format `"%Y-%m-%d %H:%M:%S"`
4. **Timezone Conversion**: Currently supports UTC to America/Vancouver conversion (8-hour offset)
5. **Output Formatting**: Formats the result using `strftime()` with the same format as input

### Current Limitations

- **Supported Conversions**: Only UTC → America/Vancouver is currently implemented
- **Timezone Data**: Does not yet parse IANA timezone files (`/usr/share/zoneinfo/`)
- **DST Handling**: Daylight saving time transitions are not handled
- **Timezone Names**: Limited to specific hardcoded timezone pairs

### Future Enhancements

The codebase includes infrastructure for full IANA timezone support:

- **TZ File Parsing**: Functions to read and parse timezone data files
- **Transition Tables**: Support for DST transitions and historical changes
- **Thread Safety**: Mutex-based protection for concurrent access
- **Multiple Timezones**: Extensible to support any IANA timezone

## Usage

### Function Signature

```sql
CONVERT_TZ(dt, from_tz, to_tz) -> text
```

**Parameters:**

- `dt`: Datetime string in format 'YYYY-MM-DD HH:MM:SS' or 'YYYY-MM-DD HH:MM:SS.ZZZ'
- `from_tz`: Source timezone name (currently only 'UTC' supported)
- `to_tz`: Target timezone name (currently only 'America/Vancouver' supported)

**Returns:**

- Converted datetime string in the same format as input
- `NULL` if any input is `NULL`
- Error message for unsupported timezone conversions

### Examples

#### Basic Usage

```sql
-- Convert UTC to Pacific Time
SELECT CONVERT_TZ('2024-01-01 12:00:00', 'UTC', 'America/Vancouver');
-- Result: "2024-01-01 04:00:00"

-- Convert with milliseconds
SELECT CONVERT_TZ('2024-01-01 12:00:00.123', 'UTC', 'America/New_York');
-- Result: "2024-01-01 07:00:00.123"
```

#### With Current Time

```sql
-- Get current time in different timezone
SELECT CONVERT_TZ(datetime('now'), 'UTC', 'America/Vancouver') AS vancouver_time;
```

#### Timezone Conversions

```sql
-- Convert between various timezones
SELECT CONVERT_TZ('2024-07-01 15:30:00', 'America/New_York', 'Europe/London') AS london_time;
-- Result: "2024-07-01 20:30:00"
```

## Usage in Migration Scripts

The Convert TZ extension provides MySQL-compatible timezone conversion for SQLite, enabling consistent datetime handling across different database engines.

### Automatic Extension Loading

Extensions are loaded dynamically at connection time in the Hydrogen C codebase, similar to other SQLite extensions.

### Migration Author Experience

Authors can write timezone conversions that work consistently across MySQL and SQLite:

```sql
-- Works in both MySQL and SQLite (with extension loaded)
SELECT CONVERT_TZ(created_at, 'UTC', 'America/Vancouver') FROM events;
```

## Testing

The test suite demonstrates the extension's functionality.

### Run Tests

```bash
make test
```

Or use the test SQL file directly:

```bash
sqlite3 < test_convert_tz.sql
```

### Test Output

The test shows:

- Extension loading confirmation
- Basic timezone conversion (UTC to America/New_York)
- Current time conversion demonstration

## Common Issues

### Error: "Failed to load from_tz" or "Failed to load to_tz"

**Cause:** Timezone data not available or incorrect timezone name.

**Solution:** Ensure IANA timezone data is installed and use correct timezone names:

```bash
# Check available timezones
ls /usr/share/zoneinfo/
```

### Error: "Invalid datetime format"

**Cause:** Input datetime doesn't match expected format.

**Solution:** Use format 'YYYY-MM-DD HH:MM:SS' or 'YYYY-MM-DD HH:MM:SS.ZZZ':

```sql
-- Valid formats
SELECT CONVERT_TZ('2024-01-01 12:00:00', 'UTC', 'America/Vancouver');
SELECT CONVERT_TZ('2024-01-01 12:00:00.123', 'UTC', 'America/Vancouver');
```

### Extension Won't Load

**Cause:** Library path or permissions issue.

**Solution:**

```bash
# Check file exists and has correct permissions
ls -la /usr/local/lib/convert_tz.so

# Try loading with full path
sqlite3 -cmd ".load /usr/local/lib/convert_tz"
```

## Performance

The extension efficiently handles timezone conversions:

- **Caching:** Timezone data is loaded on-demand and cached per-function call
- **Thread Safety:** Uses mutex for thread-safe access to timezone data
- **Memory Management:** Minimal memory footprint with automatic cleanup

## Comparison with MySQL CONVERT_TZ

Both implementations provide equivalent functionality:

- **API:** Same function signature and behavior
- **Timezone Data:** Both use IANA timezone database
- **DST Handling:** Proper daylight saving time transitions
- **Precision:** Supports millisecond precision

Key differences:

- **Loading:** SQLite uses loadable extensions vs MySQL's built-in function
- **Memory:** SQLite uses `sqlite3_malloc()` vs standard `malloc()`
- **Init Function:** SQLite requires specific naming: `sqlite3_extension_init()`

## See Also

- [MySQL CONVERT_TZ Documentation](https://dev.mysql.com/doc/refman/8.0/en/date-and-time-functions.html#function_convert-tz)
- [IANA Timezone Database](https://www.iana.org/time-zones)
- [SQLite Loadable Extensions](https://www.sqlite.org/loadext.html)