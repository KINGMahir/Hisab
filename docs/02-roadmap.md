# Development Roadmap

## Stage 2 — Baseline and repository foundation
- Preserve existing functionality.
- Establish CMake build.
- Establish tests and documentation.
- Initialize Git repository.

## Stage 3 — Database and domain refactor
- Separate database access from UI/business logic.
- Add migrations.
- Add users, roles, permissions, devices, and audit log.
- Add inventory movement ledger.

## Stage 4 — Authentication and RBAC
- Employee login/session.
- Permission checks at service layer.
- Manager/admin approval workflow.
- Secure password/PIN storage.

## Stage 5 — Qt GUI
- Professional desktop POS interface.
- Login screen.
- Dashboard.
- Billing screen.
- Product/inventory management.
- Reports and audit screens.

## Stage 6 — Automated QA
- Unit tests.
- Integration tests.
- Qt GUI tests.
- Human-like workflow automation.
- Negative/security tests.
- Test reports and artifacts.

## Stage 7 — Offline-first synchronization
- Local operation queue.
- Idempotency keys.
- Sync states and retries.
- Conflict/reconciliation strategy.
- PostgreSQL central service.

## Stage 8 — Production hardening
- Crash recovery.
- Backup/restore.
- Logging/monitoring.
- Packaging.
- CI/CD.
- Release process.
