# MailRelay Subsystem

## What is the MailRelay Subsystem?

The MailRelay subsystem is the email communication system that handles sending notifications, reports, and alerts via email to keep users and administrators informed about system activities. It acts as a reliable email relay service that receives messages from the system and forwards them through configured SMTP servers to ensure timely delivery.

## Why is it Important?

The MailRelay subsystem serves as the primary communication channel for email-based notifications in Hydrogen, making it possible to:

- **Send system alerts** - Notify administrators about important events, errors, or maintenance needs
- **Deliver user notifications** - Keep users informed about job status, system updates, and important messages
- **Handle automated reports** - Send periodic summaries, logs, and performance data via email
- **Enable communication workflows** - Support email-based interactions between the system and external parties
- **Provide reliable delivery** - Ensure messages reach their destinations even with network issues or server problems

This subsystem transforms Hydrogen into a communicative platform that can proactively inform stakeholders about system status and activities, supporting effective monitoring, maintenance, and user engagement through email channels.

## Operator and developer guide

- [MAIL_GUIDE.md](/docs/H/MAIL_GUIDE.md) — full pipeline: templates, rewrites, debounce, events, Lua `H.mail`, inbound routes, OTP, and security
- [MAILRELAY_PLAN.md](/docs/H/plans/MAILRELAY_PLAN.md) — implementation plan and working log