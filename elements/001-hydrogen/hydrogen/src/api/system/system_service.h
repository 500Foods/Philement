/*
 * System Service API module for the Hydrogen Project.
 * Provides system-level information and operations.
 */

#ifndef HYDROGEN_SYSTEM_SERVICE_H
#define HYDROGEN_SYSTEM_SERVICE_H

//@ swagger:service System Service
//@ swagger:description System-level API endpoints for monitoring, diagnostics, and information retrieval
//@ swagger:version 1.0.0
//@ swagger:tag "System Service" Provides system-level operations, monitoring, and diagnostics

// Include all system endpoint headers
#include "appconfig/appconfig.h"
#include "info/info.h"
#include "health/health.h"
#include "prometheus/prometheus.h"
#include "recent/recent.h"
#include "test/test.h"
#include "config/config.h"

#endif /* HYDROGEN_SYSTEM_SERVICE_H */