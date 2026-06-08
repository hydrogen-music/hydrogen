---
status: proposed
date: 2026-05-20
deciders: phil (theGreatWhiteShark)
---

# AD: Online import dialog design

## Context and Problem Statement

The current `SoundLibraryOnlineImportDialog` uses a `QTreeWidget` to display all
artifact types (drumkits, songs, patterns) together in one tree. The new
`OnlineImportDialog` must provide a more modern, data-rich interface that:

- Displays only one artifact type at a time (patterns OR songs OR drumkits).
- Shows tabular metadata for quick scanning.
- Provides a detail panel for full metadata inspection.
- Supports filtering by name and tags.
- Allows toggling which index sources are active.
- Supports multi-selection for batch downloads.

How should the dialog be composed?

## Decision Drivers

- Consistency with Hydrogen's recent dialog patterns (no `.ui` file but entire
  layout buildup in C++ file for maintainability).
- Must handle 300+ artifacts without performance issues.
- Must clearly communicate artifact status (new, installed, update available).
- Multi-selection is required for batch downloads.
- Must be accessible: keyboard navigation, clear visual hierarchy.

## Considered Options

1. **QTreeWidget with columns** — Reuse the tree paradigm but add columns for
   metadata.
2. **QTableView with QAbstractTableModel** — Full model/view separation with a
   custom model.
3. **QListWidget with custom item widgets** — Card-style layout per artifact.

## Decision Outcome

Chosen option: **2 — QTableView with QAbstractTableModel**.

**Rationale:**

- `QTableView` with a custom model provides clean separation between data and
  presentation. The `OnlineImporter`'s parsed `OnlineIndex` feeds directly into
  the model.
- Model/view supports efficient filtering via `QSortFilterProxyModel` — the
  search bar and type-filter are trivially implemented as proxy filters.
- Column sorting comes for free.
- `QTreeWidget` (option 1) conflates the tree structure (which is no longer
  needed since we show one type at a time) with tabular display.
- Custom item widgets (option 3) don't scale well to 300+ entries and make
  selection handling complex.

### Dialog Layout

```
┌─────────────────────────────────────────────────────────────┐
│  [Patterns ▼]  [Search: _______________]  [Sources ▼]       │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────────────────────────────────────────────┐    │
│  │ Name       │ Author     │ Tags    │ Size  │ Status  │    │
│  ├────────────┼────────────┼─────────┼───────┼─────────┤    │
│  │ Afro-Cu... │ A. Phelip..│ Afro,...│ 5 KiB │   New   │    │
│  │ Bossa N... │ B. Author  │ Latin   │ 3 KiB │Installed│    │
│  │ ...        │ ...        │ ...     │ ...   │ ...     │    │
│  └─────────────────────────────────────────────────────┘    │
│                                                             │
├─────────────────────────────────────────────────────────────┤
│  Detail Panel                                               │
│  ┌─────────────────────────────────────────────────────┐    │
│  │ Name: Afro-Cuban 1                                  │    │
│  │ Author: Albert Phelipot                             │    │
│  │ Description: Transcribed from the book "260 ..."    │    │
│  │ License: CC BY-SA                                   │    │
│  │ Tags: Afro, Cuban, 260 Drum Machine Patterns        │    │
│  │ Version: 0  |  Format: 2  |  Notes: 16              │    │
│  │ Instruments: Hi-hat Pedal, Kick, Snare Rimshot      │    │
│  │ Hash: 8f1d80d7a6ece0aff...                          │    │
│  └─────────────────────────────────────────────────────┘    │
│                                                             │
├─────────────────────────────────────────────────────────────┤
│  [Progress Bar ═══════════════════░░░░░ 60%]                │
│                                          [Cancel] [Download]│
└─────────────────────────────────────────────────────────────┘
```

### Widget Breakdown

| Widget          | Type                       | Purpose                                 |
|-----------------|----------------------------|-----------------------------------------|
| Type selector   | `QComboBox`                | Switch between Patterns/Songs/Drumkits  |
| Search bar      | `QLineEdit`                | Filter by name and tags                 |
| Source selector | `QToolButton` + popup menu | Toggle active index URLs                |
| Artifact table  | `QTableView`               | Main content; multi-select enabled      |
| Detail panel    | `QGroupBox` with labels    | Shows full metadata of selected item    |
| Progress bar    | `QProgressBar`             | Download progress (hidden until active) |
| Cancel button   | `QPushButton`              | Close dialog / abort download           |
| Download button | `QPushButton`              | Start downloading selected items        |

### Selection Behavior

- `QTableView::setSelectionMode(QAbstractItemView::ExtendedSelection)` enables
  Ctrl+Click and Shift+Click multi-selection.
- The detail panel shows metadata for the **last selected** item (current index).
- The Download button label shows count: "Download (3 selected)".

### Source Toggling

- The source selector shows a checkable list of all URLs from
  `Preferences::m_onlineRepos`.
- Unchecking a source removes its artifacts from the filtered model (via the
  proxy model's filter).
- State is ephemeral (not persisted) — all sources are active on dialog open.

### Progress Integration

- During download, the table and Download button are disabled.
- The progress bar becomes visible and is driven by
  `EventListener::onlineImportProgressEvent()`.
- Cancel aborts the current download batch (calls
  `OnlineImporter::abort()`).
- On completion (event value 101), the table re-enables, status column refreshes
  to show newly installed items.

## Consequences

- New file: `src/gui/src/OnlineImportDialog.{h,cpp}`.
- New model class: `OnlineArtifactTableModel` (can live in the same file or
  `src/gui/src/OnlineImportDialog/OnlineArtifactTableModel.{h,cpp}`).
- The old `SoundLibraryOnlineImportDialog` is deprecated but not removed in this
  first iteration (allows fallback).
