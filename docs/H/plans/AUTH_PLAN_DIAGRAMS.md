# AUTH ENDPOINTS - ARCHITECTURE DIAGRAMS

## Authentication Flow Diagram

```mermaid
sequenceDiagram
    participant Client
    participant AuthAPI
    participant Database
    participant Logger

    Note over Client,Logger: Login Flow
    Client->>AuthAPI: POST /api/auth/login
    AuthAPI->>AuthAPI: validate_login_input()
    AuthAPI->>Database: Query #001: verify_api_key()
    Database-->>AuthAPI: system_info
    AuthAPI->>Database: Query #002: check_ip_whitelist()
    Database-->>AuthAPI: whitelist_status
    AuthAPI->>Database: Query #003: check_ip_blacklist()
    Database-->>AuthAPI: blacklist_status
    AuthAPI->>Database: Query #004: log_login_attempt()
    AuthAPI->>Database: Query #005: check_failed_attempts()
    Database-->>AuthAPI: attempt_count
    AuthAPI->>AuthAPI: handle_rate_limiting()
    AuthAPI->>Database: Query #008: lookup_account()
    Database-->>AuthAPI: account_info
    AuthAPI->>AuthAPI: verify_password()
    AuthAPI->>Database: Query #017: get_user_roles()
    Database-->>AuthAPI: user_roles
    AuthAPI->>AuthAPI: generate_jwt()
    AuthAPI->>Database: Query #013: store_jwt()
    AuthAPI->>Database: Query #014: log_successful_login()
    AuthAPI->>Database: Query #015: cleanup_login_records()
    AuthAPI->>Logger: log_auth_event()
    AuthAPI-->>Client: JWT token + user info

    Note over Client,Logger: Token Renewal Flow
    Client->>AuthAPI: POST /api/auth/renew
    AuthAPI->>AuthAPI: validate_jwt_token()
    AuthAPI->>AuthAPI: generate_new_jwt()
    AuthAPI->>Database: Update JWT storage
    AuthAPI->>Logger: log_token_renewal()
    AuthAPI-->>Client: New JWT token

    Note over Client,Logger: Logout Flow
    Client->>AuthAPI: POST /api/auth/logout
    AuthAPI->>AuthAPI: validate_jwt_for_logout()
    AuthAPI->>Database: Query #019: delete_jwt()
    AuthAPI->>Logger: log_logout_event()
    AuthAPI-->>Client: Success confirmation
```

## System Architecture Diagram

```mermaid
graph TB
    subgraph "Client Layer"
        Web[Web Browser]
        Mobile[Mobile App]
        API[Third-party API]
    end

    subgraph "API Gateway Layer"
        Gateway[Hydrogen API Gateway]
        CORS[CORS Handler]
        RateLimit[Rate Limiter]
    end

    subgraph "Authentication Layer"
        AuthService[Auth Service]
        JWTHandler[JWT Handler]
        PasswordHasher[Password Hasher]
        InputValidator[Input Validator]
    end

    subgraph "Security Layer"
        IPWhitelist[IP Whitelist Checker]
        IPBlacklist[IP Blacklist Checker]
        FailedAttemptTracker[Failed Attempt Tracker]
        AuditLogger[Audit Logger]
    end

    subgraph "Data Layer"
        DQM[Database Queue Manager]
        AcuranzoDB[(Acuranzo Database)]
        QueryCache[Query Cache]
    end

    subgraph "Infrastructure Layer"
        Config[Configuration Manager]
        Secrets[Secret Manager]
        Monitoring[Monitoring System]
        Alerts[Alert Manager]
    end

    Web --> Gateway
    Mobile --> Gateway
    API --> Gateway

    Gateway --> CORS
    Gateway --> RateLimit
    RateLimit --> AuthService

    AuthService --> InputValidator
    AuthService --> PasswordHasher
    AuthService --> JWTHandler

    AuthService --> IPWhitelist
    AuthService --> IPBlacklist
    AuthService --> FailedAttemptTracker

    IPWhitelist --> DQM
    IPBlacklist --> DQM
    FailedAttemptTracker --> DQM
    AuthService --> DQM

    DQM --> AcuranzoDB
    DQM --> QueryCache

    AuthService --> AuditLogger
    AuditLogger --> DQM

    AuthService --> Config
    JWTHandler --> Secrets

    Monitoring --> AuthService
    Monitoring --> DQM
    Monitoring --> AcuranzoDB

    Alerts --> Monitoring
```

## JWT Lifecycle Diagram

```mermaid
stateDiagram-v2
    [*] --> Generated: JWT Created
    Generated --> Valid: Signature Verified
    Valid --> Active: Within Lifetime
    Active --> Expired: Exceeds max_age
    Active --> Revoked: Manual Revocation
    Valid --> Invalid: Signature Failed
    Invalid --> [*]
    Expired --> [*]
    Revoked --> [*]

    note right of Active
        - Can be renewed
        - Valid for API calls
        - Claims accessible
    end note

    note right of Expired
        - Still valid for logout
        - Cannot be renewed
        - Claims still readable
    end note

    note right of Revoked
        - Immediately invalid
        - Cannot be used
        - Logged in database
    end note
```

## Deployment Architecture Diagram

```mermaid
graph TB
    subgraph "Development"
        DevRepo[Git Repository]
        DevBuild[Build Pipeline]
        DevTest[Test Environment]
    end

    subgraph "Staging"
        StagingK8s[Kubernetes Cluster]
        StagingDB[(Staging Database)]
        StagingSecrets[Secret Management]
    end

    subgraph "Production (DOKS)"
        ProdK8s[Kubernetes Cluster]
        ProdDB[(Production Database)]
        ProdSecrets[Secret Management]
        LoadBalancer[Load Balancer]
        CDN[CDN]
    end

    subgraph "Monitoring"
        Prometheus[Prometheus]
        Grafana[Grafana]
        AlertManager[Alert Manager]
        ELK[ELK Stack]
    end

    DevRepo --> DevBuild
    DevBuild --> DevTest
    DevTest --> StagingK8s

    StagingK8s --> StagingDB
    StagingK8s --> StagingSecrets

    StagingK8s --> ProdK8s

    ProdK8s --> ProdDB
    ProdK8s --> ProdSecrets
    ProdK8s --> LoadBalancer
    LoadBalancer --> CDN

    ProdK8s --> Prometheus
    ProdK8s --> ELK

    Prometheus --> Grafana
    Prometheus --> AlertManager
