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
    "trial": "make trial: clean build with focused error/warning output, runs shutdown test on success"
  },
  "build_quality": {
    "requirements": [
      "All code changes must pass 'make trial' with no warnings or errors",
      "Tasks are not considered complete until 'make trial' runs cleanly",
      "Use 'make trial' to quickly identify and fix compilation issues",
      "All changes must pass the shutdown test (automatically run after clean trial build)",
      "Clean shutdown behavior is critical and verified for all code changes"
    ],
    "workflow": [
      "Make code changes",
      "Run 'make trial' to check for warnings/errors",
      "Fix any issues that are reported",
      "Repeat until build is clean",
      "Ensure shutdown test passes (automatically run after clean build)",
      "Only then mark task as complete"
    ]
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