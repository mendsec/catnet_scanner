# AGENTS.md — catnet-io/app

This file provides persistent context for AI coding agents working in the `catnet-io/app` repository.

---

## What this repository is

`catnet-io/app` is the cross-platform (desktop & mobile) GUI for CatNet.
Built with Wails v2 (Go backend) + React/TypeScript (frontend).
It is a pure consumer of `catnet-io/engine` — it contains zero scanning logic.

**Module path:** `github.com/catnet-io/app`  
**Binary name:** `catnet-app` (defined in `wails.json`)  
**Go version:** 1.26.4  
**Engine dependency:** `github.com/catnet-io/engine` (see `go.mod`)  
**Frontend:** React + TypeScript, bundled by Bun

---

## Architecture

```
app.go                    ← Wails App struct, startup(), all method bindings
  ├── handlers/           ← (planned refactor) one file per domain
  │   ├── scan.go         StartScan(), StopScan()
  │   ├── quicktools.go   Ping(), ReverseDNS(), GetMAC(), ScanPorts()
  │   ├── network.go      ParseRange(), GetLocalIPRange()
  │   ├── export.go       ExportResults()
  │   └── history.go      GetScans(), GetScanReport(), DeleteScan(), CompareScans()
  └── internal/
       ├── store/          SQLite scan history (migrating from engine pkg/store)
       └── diff/           Scan comparison (migrating from engine pkg/diff)

frontend/src/
  ├── App.tsx
  └── components/
       ├── ScannerView.tsx    ← main scan UI
       ├── HistoryView.tsx    ← scan history
       └── DiffView.tsx       ← scan comparison
```

### Engine API used

This app uses `pkg/scan.Engine.ScanStream` (channel-based API) from `catnet-io/engine`.
Events flow: `ScanStream` → `chan events.Event` → goroutine drains channel →
`runtime.EventsEmit` → Wails WebSocket → React frontend.

Do NOT add a second event path. Do NOT call `engine.StartScan` (callback API) from here.

### Visual identity

See `.jules/palette.md` for the exact color palette, typography, and design tokens.
The UI follows a **cyberpunk / high-contrast glassmorphism** aesthetic.
Do not introduce flat or material design components.

---

## Hard rules — never violate

1. **No scanning logic in this repository.** All scanning happens in `catnet-io/engine`.
2. **Do not sanitize `ScanProfile` in `app.go`.** The engine calls `profile.Sanitize()`
   internally. Double sanitization causes silent divergence. Remove any manual bounds checks
   on `Concurrency` or `TimeoutMs` from Go code.
3. **No CGO.** `wails build` handles platform-specific linking; do not introduce CGO manually.
4. **English only** in all Go source files and Wails bindings.
5. **No local `replace` directives in `main` branch.** Use `scripts/dev-replace.sh`.
6. **Do not add features to `pkg/store` or `pkg/diff` in `catnet-io/engine`.**
   These packages are migrating to `internal/store` and `internal/diff` in this repository.
7. **`.archive-notice.md` belongs in `docs/`.** Do not put archive notices in the root.
8. **`CHANGES.md` is deprecated.** All changelog entries go to `CHANGELOG.md` only.

---

## Planned refactor — Sprint 5 (do if unblocked)

- Extract `app.go` method groups into `handlers/` package (one file per domain)
- Move `pkg/store` and `pkg/diff` imports from `catnet-io/engine` to `internal/store` and
  `internal/diff` in this repository (coordinate with engine Sprint 3)
- Update `go.mod` to `engine@v0.5.1` after store/diff removal from engine

---

## Conventions

### Commit messages — Conventional Commits

```
feat(scanner): add host side panel on row click
fix(app): remove duplicate ScanProfile sanitization
chore(deps): update engine to v0.5.1
refactor(app): extract scan handler to handlers/scan.go
style(frontend): update ScannerView to use palette token --neon-cyan
```

Scopes: `scanner`, `history`, `diff`, `export`, `quicktools`, `network`, `app`,
`frontend`, `deps`, `ci`, `docs`.

### Changelog — Keep a Changelog

Update `CHANGELOG.md` under `[Unreleased]` for every behavioral change.
`CHANGES.md` is deprecated — do not add entries there.

### Frontend conventions

- TypeScript strict mode — no `any` types.
- All Wails backend calls via the generated bindings in `frontend/src/wailsjs/`.
- Do not call `fetch()` or `axios` for backend data — use Wails runtime only.
- Use CSS variables from `.jules/palette.md` for all color values.
- Components live in `frontend/src/components/` — one file per view.

---

## CI requirements — all must pass before merge

- Go: `go build ./...`, `go vet ./...`
- Frontend: `cd frontend && npm run build`
- Wails: `wails build` (on release PRs)
- Security: `semgrep`, `snyk`, `govulncheck` (configured as required checks)

---

## catnet-scanner — Session Summary (Legacy Context)

### Goal
- Harden `mendsec/catnet-scanner` with DevSecOps practices (SHA pinning, permissions, Semgrep SAST) and automate signed PRs from `develop` to `main` via SSH-signed commits on `develop-signed`.

### Constraints & Preferences
- PR author must be `github-actions[bot]` (not `mendsec`) so the user can review and merge.
- Commits on `develop-signed` must show **Verified** badge (SSH signing key added to GitHub account).
- Follow the pattern from the `mendsec/catnet` repo (`auto-merge-pr.yml` + `BOT_SSH_PRIVATE_KEY`).
- CI must work end-to-end: `catnet-core` private dependency must resolve in CI.

### Progress
#### Done
- All 4 workflows (`ci.yml`, `govulncheck.yml`, `release.yml`, `snyk.yml`): added `permissions: contents: read` (with override on `release` job to `write`), pinned 14 third-party actions by commit SHA.
- Removed floating tags (`@v4`, `@v6`, `@v1`, `@master`, etc.) across all workflows.
- Created `.github/dependabot.yml` (weekly schedule, github-actions ecosystem).
- Created `.github/workflows/semgrep.yml` (SAST for Go + TS, pinned to `713efdd`).
- Created `.github/PULL_REQUEST_TEMPLATE.md` (Security, Accessibility, Testing, Risk sections).
- Updated `CONTRIBUTING.md` with full DevSecOps Guide.
- Created `.github/workflows/signed-merge.yml` (triggers on push to `develop` and `workflow_dispatch`): SSH-signs commits via `git filter-branch -S`, pushes to `develop-signed`, closes old PRs, opens new PR to `main`.
- SSH signing key generated (no passphrase), added as `BOT_SSH_PRIVATE_KEY` secret, public key added to GitHub account as signing key.
- 4 dependabot PRs (#70–#73) approved and merged to `main`: updated `action-gh-release`, `setup-go`, `checkout`, `setup-bun` to latest versions.
- `GH_PAT` added as a Dependabot secret on `catnet-scanner` so dependabot-triggered CI can checkout private `catnet-core`.
- Aikido PR #63 closed as superseded (action pinning already done).
- Palette PR #62 merged to `develop`: table header keyboard accessibility (`tabIndex`, `onKeyDown`, `aria-sort`, `handleSortKeyDown`, `th:focus-visible` CSS).
- Changes from PRs #41 (empty state scanning message), #42 (scan input Enter key handler), #43 (progress bar ARIA attributes) applied directly to `develop` via commit `51f0764`.
- Remaining 18 palette PRs (#41–#60) closed as superseded.
- Signed-merge workflow sync step updated: handles divergent branches by merging `main` into `develop` instead of a plain fast-forward push.
- PRs #69, #74, #75, #76, #77 signed-merge completed: `develop-signed` → `main` (all 9 CI checks passed).

#### In Progress
- (none)

#### Blocked
- (none)

### Key Decisions
- Use SSH signing (`BOT_SSH_PRIVATE_KEY`) instead of GPG — matches the proven catnet repo pattern.
- Use `GITHUB_TOKEN` for PR creation — makes the author `github-actions[bot]`.
- The `if: github.actor != 'github-actions[bot]'` guard prevents re-triggering loops on the signed push.
- Proxy packages in catnet-core bridge import gap rather than restructuring catnet-scanner's code.
- Dependabot PRs merged despite CI infra failures (secrets not available to dependabot actor); `GH_PAT` added as Dependabot secret to fix long-term.
- Palette a11y improvements consolidated into a single merged PR (#62) + direct commits instead of 19 conflicting PRs.

### Next Steps
1. Remove the `GH_PAT` secret if no longer needed elsewhere.
2. Continue monitoring signed-merge automation for regressions on future `develop` pushes.

### Critical Context
- The `GITHUB_TOKEN` restriction ("GitHub Actions is not permitted to create or approve pull requests") is a repo-level setting that the user enabled — both REST and GraphQL now work.
- `git filter-branch -S` rewrites all commits from `origin/main..HEAD` with the SSH signing key.
- Commits on `develop-signed` branches show `verified: true` for all rewritten commits.
- The `actions/checkout@v4` deprecation warning (Node 20 → 24) is cosmetic.
- `GH_PAT` Dependabot secret was created on the repo settings page to allow dependabot-triggered CI to access private `catnet-core`.
- The signed-merge sync step now uses `git merge origin/main` instead of `git push origin origin/main:develop` to handle divergent branches.

### Relevant Files
- `.github/workflows/signed-merge.yml`: signed-merge automation with updated sync step (merge instead of fast-forward push)
- `.github/workflows/ci.yml`: CI with SHA-pinned actions, permissions, and `GH_PAT` for catnet-core checkout
- `.github/workflows/govulncheck.yml`: vulnerability scanning with SHA-pinned actions
- `.github/workflows/release.yml`: release with explicit `contents: write` override on release job
- `.github/workflows/snyk.yml`: Snyk scanning, switched `@master` to `@v1.0.0`
- `.github/workflows/semgrep.yml`: SAST workflow
- `.github/dependabot.yml`: weekly dependabot config
- `.github/PULL_REQUEST_TEMPLATE.md`: PR template with DevSecOps checklist
- `CONTRIBUTING.md`: appended DevSecOps Guide section
- `mendsec/catnet-core` repo: `/pkg/scan`, `/pkg/events`, `/pkg/export`, `/pkg/profile`, `/pkg/results` — proxy packages added for catnet-scanner compatibility
- `frontend/src/App.tsx`: monolithic component with keyboard-accessible sortable table headers, scan input Enter key, progress bar ARIA, and dynamic empty state message
