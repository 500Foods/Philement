# Hydrogen Build Metrics Browser Architecture

## System Overview

```mermaid
flowchart TD
    A[Start] --> B{Execution Mode}
    B -->|Browser| C[Load hbm_browser.html]
    B -->|Command Line| D[Run hbm_browser.js with config]

    C --> E[Load D3, Flatpickr, Font Awesome from CDN]
    C --> F[Load hbm_browser.css]
    C --> G[Load hbm_browser.js]

    D --> H[Load configuration from JSON]
    D --> I[Process metrics files]
    D --> J[Generate SVG output]

    G --> K[Discover Metrics Files]
    E --> K
    H --> K

    K --> L[Extract Numeric Values]
    L --> M[Create Flattened Value List]

    M --> N[Render Interactive UI]
    M --> O[Render D3 Chart]

    N --> P[User Interaction: Add/Remove Elements]
    N --> Q[User Interaction: Change Date Range]
    N --> R[User Interaction: Modify Chart Settings]

    P --> O
    Q --> K
    R --> O

    O --> S[Display Chart with Dual Axes]
    J --> T[Save SVG File]

    S --> U[End]
    T --> U
```

## Component Details

### 1. File Discovery System

- **Local Mode**: `/docs/H/metrics/{year-month}/{year-month-day}.json`
- **GitHub Mode**: `https://github.com/500Foods/Philement/docs/H/metrics/{year-month}/{year-month-day}.json`
- **Fallback Logic**: Try today's date, then go back day-by-day until file is found

### 2. Data Processing Pipeline

```mermaid
flowchart LR
    A[Raw JSON] --> B[Flatten Nested Structure]
    B --> C[Extract Numeric Values]
    C --> D[Create Value Paths]
    D --> E[Filter by Date Range]
    E --> F[Prepare for Charting]
```

### 3. Chart Configuration

- **Dual Axes**: Left/Right axis selection per metric
- **Chart Types**: Line/Bar selection per metric
- **Styling**: Color, label, and legend customization
- **Date Range**: Configurable start/end dates

### 4. User Interface Components

- **Main Chart Area**: Full-window SVG display
- **Control Panel**: Add/remove metrics, modify settings
- **Date Pickers**: Flatpickr for start/end date selection
- **Color Picker**: Simple JS color picker integration

### 5. Command Line Interface

- **Input**: Configuration JSON file path
- **Output**: SVG file path
- **Processing**: Headless chart generation