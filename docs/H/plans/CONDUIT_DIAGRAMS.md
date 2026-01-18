# Conduit Service Architecture Diagrams

## Request Flow Architecture

```mermaid
sequenceDiagram
    participant Client
    participant API as Conduit API
    participant QTC as Query Cache
    participant QS as Queue Selector
    participant DQM as Database Queue
    participant DB as Database

    Client->>API: POST /api/conduit/query
    API->>QTC: Lookup query_ref
    QTC-->>API: SQL template + metadata
    API->>API: Parse typed parameters
    API->>API: Convert named params to ?
    API->>QS: Find best queue
    QS->>QS: Check queue depths
    QS->>QS: Check timestamps
    QS-->>API: Selected DQM
    API->>DQM: Submit query + wait
    DQM->>DB: Execute query
    DB-->>DQM: Result
    DQM-->>API: Wake thread + return
    API-->>Client: JSON result
```

## Component Architecture

```mermaid
graph TB
    API[Conduit API Endpoint]
    QTC[Query Table Cache]
    PS[Parameter Service]
    QS[Queue Selector]
    PR[Pending Results]
    DQM[Database Queue Manager]

    API -->|1. Lookup query_ref| QTC
    API -->|2. Parse params| PS
    PS -->|Named to positional| PS
    API -->|3. Select best queue| QS
    QS -->|Check depths/times| DQM
    API -->|4. Submit + register| PR
    API -->|5. Submit query| DQM
    API -->|6. Wait with timeout| PR
    DQM -->|7. Signal complete| PR
    PR -->|8. Return result| API
```

## Request Processing Flow

```mermaid
graph TB
    A[Client POST Request] --> B[Parse JSON Body]
    B --> C[Lookup query_ref in QTC]
    C --> D[Parse Typed Parameters]
    D --> E[Convert Named to Positional]
    E --> F[Select Optimal DQM]
    F --> G[Register Pending Result]
    G --> H[Submit to DQM Queue]
    H --> I[Wait with Timeout]
    I --> J[Return Result or Error]
```

## DQM Processing Flow

```mermaid
graph TB
    A[DQM Worker Dequeues] --> B[Deserialize Query]
    B --> C[Acquire DB Connection]
    C --> D[Execute Query]
    D --> E[Serialize Result]
    E --> F[Signal Pending Result]
    F --> G[Release Connection]
```

## Parallel Query Execution for Multiple Endpoints

```mermaid
graph TB
    A[Client Request] --> B[Parse Query Array]
    B --> C[Create Pending Results Array]
    C --> D[Submit Each Query to DQM]
    D --> E[Suspend Webserver Thread]
    E --> F[Wait for All Results]
    F --> G[Aggregate Results]
    G --> H[Resume & Return Response]

    D --> I[Query 1 → DQM A]
    D --> J[Query 2 → DQM B]
    D --> K[Query 3 → DQM C]

    I --> L[Parallel Execution]
    J --> L
    K --> L

    L --> M[Signal Completion]
    M --> F
```

## Webserver Resource Suspension

```mermaid
stateDiagram-v2
    [*] --> RequestReceived
    RequestReceived --> ParseRequest
    ParseRequest --> SubmitQueries
    SubmitQueries --> SuspendThread: Free webserver resources
    SuspendThread --> WaitForResults: Block on condition variables
    WaitForResults --> CheckTimeouts: Periodic timeout checks
    CheckTimeouts --> ResultsReady: All queries complete
    CheckTimeouts --> TimeoutError: Timeout exceeded
    ResultsReady --> ResumeThread: Reacquire webserver resources
    TimeoutError --> ResumeThread
    ResumeThread --> FormatResponse
    FormatResponse --> [*]