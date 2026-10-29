# Architecture Direction

## 1. Production POS

```text
Authenticated human
        |
        v
   Qt desktop POS
        |
        +---- Local SQLite (primary operational database)
        |
        +---- Sync queue
        |
        +---- Optional online API
```

The application must continue normal shop operations when the network is unavailable.

## 2. Authorization model

The future production system will use RBAC with roles such as employee, supervisor, manager, and administrator. Permissions will be explicit rather than relying on hidden UI restrictions.

Examples:

- Employee: create sales and accept payments.
- Supervisor: approve selected operational exceptions.
- Manager: manage products, pricing, stock adjustments, purchases, and approvals according to assigned permissions.
- Administrator: manage users, roles, configuration, and system-level controls.

## 3. Auditability

Important business events will record at minimum:

- actor/user ID
- action
- entity/reference ID
- timestamp
- device/terminal ID where applicable
- result
- approval information where applicable

Inventory will evolve toward a movement ledger rather than unexplained direct quantity edits.

## 4. Online synchronization

The future central service will use an API and PostgreSQL. Local SQLite databases will not be copied or merged as files. Changes will be synchronized as idempotent business events/operations.

## 5. Automated QA environment

The automated tester is separate from the production POS. It will drive the GUI and application through controlled test accounts and test data, simulating employees and managers.

Planned coverage:

- authentication
- authorization
- billing
- inventory consistency
- approvals
- audit trails
- offline operation
- network interruption
- synchronization/retry
- crash recovery
- regression testing
- security/negative tests
