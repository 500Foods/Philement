# HYDROGEN

🚨 **CRITICAL BUILD REQUIREMENT** 🚨
Before marking ANY code changes as complete:

1. Run `make trial`
2. Ensure NO warnings or errors
3. Verify shutdown test passes
4. Only then consider the task done

⚠️ This is a mandatory step - no exceptions! See "build_quality" section below for details.

```json
{
  "project": {
    "name": "Hydrogen",
    "language": "C",
    "type": "3D Printer Control Server",
    "architecture": "Multithreaded",
    "key_tech": ["POSIX", "HTTP/WebSocket", "mDNS", "Queues", "JSON", "OIDC", "OpenSSL"],
    "critical": ["TZ:America/Vancouver", "ThreadSafety", "ErrorHandling", "CodeOrg"]
  },
  "code_structure": {
    "file_org": ["DocHeader", "FeatureTest", "Includes", "ExternDecl", "PublicAPI", "PrivateDecl", "Implementation"],
    "critical": ["ThreadSafety", "ErrorHandling", "WhyDocumentation", "MutexMgmt"]
  },
  "ai_comments": {
    "function": "/**\n * @brief Purpose\n * @param name Description\n * @returns value\n * @note AI-guidance: ThreadSafety/ErrorHandling notes\n */",
    "markers": ["AI:IGNORE_START/END", "AI:IMPORTANT", "AI:REFACTOR_CANDIDATE"],
    "practices": ["BeExplicit", "ProvideContext", "ConsistentFormat", "UpdateComments", "LinkRelated", "ShowExamples", "NoteConstraints"]
  },
  "security": {
    "token": "validate(format,signature,issuer,audience,expiration)",
    "crypto": ["UseEstablishedLibs", "NoCustomAlgo", "SecureKeyMgmt", "ModernAlgorithms", "SecureRNG"],
    "validation": "WhitelistValidation",
    "errors": "DefaultToSecure"
  },
  "key_files": [
    "src/hydrogen.c",
    "docs/developer_onboarding.md",
    "docs/coding_guidelines.md",
    "docs/api.md",
    "docs/README.md",
    "tests/README.md"
  ],
  "code_patterns": {
    "error": "if(error){log_this(component,msg,level,outputs);cleanup();return false;}",
    "thread": "pthread_mutex_lock(&mutex);/*access*/pthread_mutex_unlock(&mutex);",
    "memory": "ptr=malloc(size);if(!ptr){log_this(component,msg,4,out);return false;}",
    "arch_doc": "/*Why:Safety-Critical(startup,shutdown),RealTime(response,monitoring)*/"
  },
  "logging": {
    "levels": {
      "1": "INFO: operational info, startup messages",
      "2": "WARN: warning conditions, non-blocking issues",
      "3": "DEBUG: detailed debug info for troubleshooting",
      "4": "ERROR: affects functionality but not shutdown",
      "5": "CRITICAL: requires immediate attention"
    },
    "defaults": {"console":1, "db":4, "file":1},
    "never_use": "level 0 (special filter value)"
  },
  "json_format": {
    "percentages": "Always string with 3 decimal places: \"12.345\"",
    "bytes": "Always integers",
    "time": "Always strings for durations and timestamps"
  },
  "build_variants": {
    "default": "make: standard optimization - build/",
    "debug": "make debug: debug symbols - build_debug/",
    "perf": "make perf: max optimization/LTO - build_perf/",
    "release": "make release: production - build_release/",
    "valgrind": "make valgrind: memory analysis - build_valgrind/",
    "trial": "make trial: clean build with focused error/warning output (must be run from src/ directory), runs shutdown test on success"
  },
  "build_quality": {
    "requirements": [
      "All code changes must pass 'make trial' with no warnings or errors",
      "Tasks are not considered complete until 'make trial' runs cleanly",
      "Use 'make trial' to quickly identify and fix compilation issues",
      "All changes must pass the shutdown test (automatically run after clean trial build)",
      "Clean shutdown behavior is critical and verified for all code changes",
      "New features must include appropriate tests using the standardized test template"
    ],
    "workflow": [
      "Make code changes",
      "Create/update tests using test_template.sh as guide",
      "Run 'make trial' to check for warnings/errors",
      "Fix any issues that are reported",
      "Run relevant test suite",
      "Repeat until build is clean and tests pass",
      "Ensure shutdown test passes (automatically run after clean build)",
      "Only then mark task as complete"
    ],
    "testing": {
      "template": "tests/test_template.sh provides standardized patterns for test creation",
      "patterns": [
        "Basic startup/shutdown validation",
        "API endpoint testing",
        "Signal handling",
        "Resource monitoring",
        "Configuration validation"
      ],
      "best_practices": [
        "Use test_template.sh for consistent test structure",
        "Follow test numbering convention (00-99)",
        "Include both minimal and maximal configuration testing",
        "Monitor resource usage during tests",
        "Verify clean shutdown behavior",
        "Document test purpose and requirements"
      ],
      "documentation": [
        "See tests/README.md for detailed testing guide",
        "Review docs/testing.md for testing philosophy",
        "Check existing tests for examples",
        "Use test_template.sh comments for guidance"
      ]
    }
  },
  "linting": {
    "tools": {
      "C": "cppcheck",
      "MD": "markdownlint",
      "JSON": "jsonlint",
      "JS": "eslint",
      "CSS": "stylelint",
      "HTML": "htmlhint"
    },
    "configs": [".lintignore", ".lintignore-markdown"],
    "run": ["./tests/test_all.sh", "./tests/test_z_codebase.sh"]
  },
  "common_issues": {
    "thread": ["mutex_consistency", "cond_vars", "shutdown_sequence", "tests/analyze_stuck_threads.sh"],
    "memory": ["valgrind_build", "resource_cleanup", "error_path_cleanup", "tests/monitor_resources.sh"],
    "config": ["json_syntax", "file_permissions", "path_verification", "param_testing"]
  },
  "doc_structure": {
    "dirs": ["reference: technical docs", "guides: user docs", "development: dev docs"],
    "types": ["reference: technical", "guides: task-oriented", "development: standards"],
    "style": ["clear_title", "hierarchical_headings", "code_with_syntax", "relative_links"]
  },
  "release_notes": {
    "levels": ["RELEASES.md: chronological index", "docs/releases/YYYY-MM-DD.md: daily details"],
    "format": ["Pacific Time", "Technical Focus", "Grouped Topics", "Nested Bullets", "Factual", "NoAdjectives"],
    "topics": ["API", "WebSocket", "WebServer", "Testing", "Documentation", "Configuration", "General"],
    "workflow": ["Create daily file", "Add changes", "Update index", "Verify TZ"]
  },
  "terms": {
    "Queue": "Thread-safe data structure for communication",
    "Service": "Standalone component with thread(s)",
    "Handler": "Request processing function",
    "Context": "Service state data structure",
    "Message": "Inter-component data packet",
    "Endpoint": "HTTP API access point",
    "Resource": "Managed system component"
  },
  "launch_system": {
    "overview": "The launch system manages subsystem initialization through a carefully orchestrated process",
    "quick_reference": {
      "core_concepts": [
        "Subsystem Registry is first - it manages all other subsystems",
        "Launch code (launch.c) is lightweight orchestration only",
        "Subsystem code lives in separate launch-*.c files",
        "Dependencies determine order, no subsystem hierarchy",
        "Landing (in landing/*.c) mirrors launch in reverse"
      ],
      "standardized_order": {
        "description": "When listing or processing subsystems, always use this standard order:",
        "order": [
          "1. Subsystem Registry (manages all other subsystems)",
          "2. Payload",
          "3. Threads",
          "4. Network",
          "5. Database",
          "6. Logging",
          "7. WebServer",
          "8. API",
          "9. Swagger",
          "10. WebSockets",
          "11. Terminal",
          "12. mDNS Server",
          "13. mDNS Client",
          "14. MailRelay",
          "15. Print"
        ]
      },
      "file_organization": {
        "launch.c": "Main orchestration and coordination",
        "launch-*.c": "One file per subsystem with specific launch code",
        "launch.h": "Public interface and shared structures",
        "landing/*.c": "Mirror of launch files for shutdown"
      },
      "key_functions": {
        "check_all_launch_readiness": "Phase 1: Verify subsystems can start",
        "handle_launch_plan": "Phase 2: Create launch sequence",
        "launch_approved_subsystems": "Phase 3: Start in dependency order",
        "handle_launch_review": "Phase 4: Verify launch success"
      },
      "subsystem_files": {
        "launch-threads.c": "Thread management subsystem",
        "launch-network.c": "Network connectivity subsystem",
        "launch-webserver.c": "Web server subsystem",
        "(etc)": "One file per subsystem"
      }
    },
    "phases": {
      "1_readiness": {
        "what": "Check readiness of all subsystems",
        "how": "Each subsystem has check_*_launch_readiness() function",
        "where": "Implemented in respective launch-*.c files",
        "first": "Subsystem Registry checked first, must pass"
      },
      "2_planning": {
        "what": "Create launch sequence based on dependencies",
        "how": "Use registry to determine correct order",
        "where": "Coordinated in launch.c",
        "key": "Dependencies determine order, not importance"
      },
      "3_execution": {
        "what": "Launch subsystems that passed readiness",
        "how": "Call subsystem-specific launch functions",
        "where": "Code in respective launch-*.c files",
        "order": "Based on dependencies via registry"
      },
      "4_review": {
        "what": "Verify successful launch of all subsystems",
        "how": "Check final states and collect metrics",
        "where": "Coordinated in launch.c",
        "result": "System ready for operation"
      }
    },
    "subsystem_organization": {
      "registry_first": "Subsystem Registry initializes first as it manages all other subsystems",
      "code_separation": {
        "orchestration": "src/launch/launch.c coordinates the overall process",
        "subsystems": "src/launch/launch-*.c contains subsystem-specific code",
        "readiness": "Each subsystem has its own launch readiness check"
      }
    }
  },
  "launch_readiness": {
    "overview": "Subsystem launch readiness checks verify all prerequisites are met before initialization",
    "importance": ["Prevents cascading failures", "Ensures clean startup", "Provides clear status reporting", "Facilitates dependency management"],
    "function_structure": {
      "signature": "LaunchReadiness check_SUBSYSTEM_launch_readiness(void)",
      "return": "LaunchReadiness struct with subsystem name, ready status, and message array",
      "pattern": ["Check system state", "Verify configuration", "Check dependencies", "Validate resources", "Build message array", "Return readiness struct"]
    },
    "common_checks": {
      "system_state": "Check server_stopping, server_starting, server_running flags to prevent initialization during shutdown",
      "configuration": "Verify required configuration is loaded and valid",
      "dependencies": "Check that required subsystems are running",
      "resources": "Verify access to required files, network resources, etc.",
      "environment": "Check environment variables and system capabilities"
    },
  "landing_relationship": {
      "overview": "Landing process mirrors launch in reverse order with comprehensive status tracking",
      "implementation": "src/landing/ contains mirror implementations of launch files",
      "purpose": "Ensures clean shutdown by freeing resources in correct order with verification",
      "structure": "Follows same separation of concerns as launch system",
      "phases": {
        "1_status_assessment": {
          "what": "Evaluate current system state",
          "how": "Count ready/not-ready subsystems",
          "where": "Coordinated in landing_plan.c",
          "key": "Verify at least one subsystem is ready for landing"
        },
        "2_dependency_analysis": {
          "what": "Create safe landing sequence",
          "how": "Check each subsystem's dependents",
          "where": "Implemented in landing_plan.c",
          "key": "Ensure dependents are landed or inactive before proceeding"
        },
        "3_go_no_go": {
          "what": "Make final landing decision",
          "how": "Evaluate readiness and dependencies",
          "where": "Coordinated in landing_plan.c",
          "key": "All dependencies must be satisfied"
        },
        "4_review": {
          "what": "Comprehensive landing verification",
          "how": [
            "Calculate total landing duration",
            "Verify thread cleanup completion",
            "Report landing success rate",
            "Detail each subsystem's final state"
          ],
          "where": "Implemented in landing_review.c",
          "metrics": [
            "Landing duration in seconds",
            "Thread cleanup status",
            "Landing success rate percentage",
            "Individual subsystem states"
          ]
        }
      },
      "status_reporting": {
        "timing": "Landing duration tracked and reported",
        "threads": "Active thread count verified",
        "success_rate": "Percentage of successfully landed subsystems",
        "state_detail": "Final state of each subsystem logged"
      }
    },
    "message_format": {
      "subsystem_name": "First message is just the subsystem name",
      "go_messages": "  Go:      Check Description (details)",
      "no_go_messages": "  No-Go:   Check Description (reason for failure)",
      "decide_message": "  Decide:  Go/No-Go For Launch of SUBSYSTEM Subsystem"
    },
    "best_practices": [
      "Mirror actual initialization logic in readiness checks",
      "Use get_executable_path() instead of hardcoded paths",
      "Check system state flags first and return early if in shutdown",
      "Keep output concise and focused on essential information",
      "Validate all critical dependencies",
      "Free all allocated resources before returning",
      "Use consistent spacing in message formatting"
    ],
    "implementation_steps": [
      "Study the subsystem's initialization code to understand prerequisites",
      "Identify all external dependencies (other subsystems, files, etc.)",
      "Create a check_SUBSYSTEM_launch_readiness function in the appropriate file",
      "Implement checks in order of importance (system state, config, dependencies, etc.)",
      "Add the function to launch.c in the check_all_launch_readiness function",
      "Test with both valid and invalid configurations"
    ],
    "example_files": [
      "src/config/launch/launch-payload.c: Payload subsystem",
      "src/config/launch/launch.c: Main launch readiness orchestration"
    ]
  }
}
```

## Configuration System

### Configuration Loading Order

The configuration system follows the standard subsystem order, with the "server" section being treated as equivalent to registry in priority. The loading sequence is:

1. Server/Registry (core system settings)
2. Payload
3. Threads
4. Network
5. Database
6. Logging
   - Core logging configuration
   - Notify (logging output path, like Database/Console/File outputs)
7. WebServer
8. API
9. Swagger
10. WebSockets
11. Terminal
12. mDNS Server
13. mDNS Client
14. MailRelay
15. Print

This order is maintained across:

- Configuration file processing
- Default value assignment
- Environment variable resolution
- Documentation and code organization

Note: While Database appears both as a subsystem (#5) and as a logging output path, this reflects its dual role in the system. As a subsystem, it provides core database services. As a logging output, it's one of several possible destinations for log messages, alongside Console, File, and Notify outputs.

### Configuration Sources (In Priority Order)

1. Environment Variables (HYDROGEN_CONFIG for file path, subsystem-specific vars)
2. Command Line Specified Config File
3. Standard Locations:
   - ./hydrogen.json
   - /etc/hydrogen/hydrogen.json
   - /usr/local/etc/hydrogen/hydrogen.json
4. Built-in Defaults

### Default Values and Environment Variables

Each configuration section has built-in defaults that are used when:

- The configuration file is missing
- A section is missing from the config file
- Individual settings are omitted

Environment variables can override any configuration value using:

- Direct environment variables (e.g., HYDROGEN_SERVER_NAME)
- ${env.VARIABLE_NAME} syntax in JSON values

### Server Configuration Priority

The "server" section in configuration files is treated with the same priority as the registry subsystem (first). This section contains core system settings that affect all other subsystems, including:

- Server identification
- File paths
- System-wide timeouts
- Core security settings

## Subsystem Order

When listing or processing subsystems, the following order must be maintained:

1. Subsystem Registry (manages all other subsystems)
2. Payload
3. Threads
4. Network
5. Database
6. Logging
7. WebServer
8. API
9. Swagger
10. WebSockets
11. Terminal
12. mDNS Server
13. mDNS Client
14. MailRelay
15. Print

This order reflects the dependency chain and must be followed in:

- Code organization (includes, declarations)
- Launch sequence
- Configuration processing
- Documentation
- Any other context where subsystems are listed or processed

## REPOSITORY MAP

```diagram
hydrogen/
├── src/                # Source code
│   ├── api/            # API endpoints (oidc/,system/)
│   ├── config/         # Configuration
│   ├── logging/        # Logging system
│   ├── mdns/           # Service discovery
│   ├── network/        # Network interfaces
│   ├── oidc/           # OpenID Connect
│   ├── print/          # 3D printing
│   ├── queue/          # Thread-safe queues
│   ├── state/          # System state
│   ├── utils/          # Utilities
│   ├── webserver/      # HTTP server
│   ├── websocket/      # WebSocket server
│   └── hydrogen.c      # Main entry
├── docs/               # Documentation
├── tests/              # Test framework
├── build*/             # Build directories
└── oidc-client-examples/ # OIDC examples
```

## CODING EXAMPLES

```c
// Error handling
if(error){log_this("Component","Error details",4,true,true,true);cleanup();return false;}

// Thread safety
pthread_mutex_lock(&mutex); /* critical section */ pthread_mutex_unlock(&mutex);

// Memory management
ptr=malloc(size);if(!ptr){log_this("Comp","Alloc failed",4,true,true,true);return false;}

// Security validation
if(!validate_all(token,issuer,audience,expiry)){log_this("Auth","Invalid",4,1,1,1);return AUTH_ERROR;}

// Logging (levels: 1=INFO, 2=WARN, 3=DEBUG, 4=ERROR, 5=CRITICAL, NEVER use 0)
log_this("Component","Message details",severity,console,file,websocket);

// JSON formatting (percentages as 3-decimal strings, integers for bytes)
snprintf(percent_str,sizeof(percent_str),"%.3f",value);
json_object_set_new(obj,"percent",json_string(percent_str));
json_object_set_new(obj,"bytes",json_integer(bytes));