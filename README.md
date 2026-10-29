# POS Inventory System

A professional C++ POS and inventory system being developed from an existing SQLite-based POS application.

## Project direction

The target system is **offline-first**, authenticated, auditable, role-based, and suitable for commercial shop operations.

### Core principles

- POS operation is performed by authenticated humans.
- Employees must authenticate before creating transactions.
- Role-based permissions control sensitive operations.
- Every sale is attributable to an employee.
- Inventory changes are traceable through an inventory movement ledger.
- Price and stock changes require appropriate authorization.
- Audit logs record important actions and approvals.
- The local SQLite database remains operational without internet access.
- A future central server will synchronize approved local changes.
- A separate automated QA environment will operate the POS like a human user and test normal, unauthorized, offline, recovery, and failure scenarios.

## Current stage

Stage 2 establishes a clean, version-controlled foundation while preserving the original application behavior in `src/legacy/main.cpp`.

Authentication/RBAC, inventory ledger, audit logging, Qt GUI, synchronization, and automated GUI testing are intentionally implemented in later stages rather than mixed into the first migration.

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/pos_inventory
```

## Repository policy

The repository should be private during development. Do not commit real passwords, PINs, API keys, production databases, customer data, or other secrets.
