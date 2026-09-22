# Repository Guide

## Constraints & Boundaries
- **Strictly Local Operations**: All file searches, reads, creations, edits, and tool executions must be confined strictly within the repository root directory (`/media/kartashoff/Storage/temp/BTE`). Leaving the repository directory or modifying/accessing external files is strictly forbidden.

## Structure
- **`ghidra/`**: Ghidra reverse engineering framework.
- **`Driver/`**: Consolidated Apple Broadcom Bluetooth driver installation package and binaries (`AppleBTBC.inf`, `.sys`, `.cat`).

## Build & Run (Ghidra)
- **Prerequisites**: Java 21 JDK (required on `PATH` or `JAVA_HOME`); Python 3.9–3.14 (for Debugger and PyGhidra).
- **Gradle Wrapper Location**: `ghidra/support/gradle/` (non-standard root).
- **Build Native Components**:
  ```bash
  cd ghidra/support/gradle
  ./gradlew buildNatives
  ```
- **Execution**:
  - GUI: `./ghidraRun` (Linux/macOS) or `ghidraRun.bat` (Windows) inside `ghidra/`.
  - Headless: `ghidra/support/analyzeHeadless`.

## Reverse Engineering Infrastructure & Workflows

| Task | Tool / Approach |
| :--- | :--- |
| **Exact bytes / instructions** | `radare2` |
| **Strings, raw xrefs** | `radare2` |
| **Fast CLI queries** | `radare2` |
| **Bulk JSON export** | `r2pipe + Python` |
| **Control Flow Graph (CFG)** | `Ghidra + r2` (compare) |
| **Call graph** | `Ghidra` |
| **Argument definition** | `Ghidra` |
| **Type recovery** | `Ghidra` |
| **Pseudocode** | `Ghidra` |
| **Semantic function comparison** | `Ghidra P-code / BSim` |
| **Cross-binary function matching (wl7 / wl10 / Linux)** | `Ghidra + custom metrics` |
| **Verifying controversial results** | `radare2` |

## Session Logging (Token-Efficient)
- Use `./log_action.py "description of action"` to log actions, queries, and analysis findings continuously to `logs/session.log` without cluttering LLM context.

