# catnet-io/app

**CatNet App** is the graphical frontend of the CatNet ecosystem — a cross-platform desktop
application (Wails + React) for network discovery and scan workflows, built for users who want
a more accessible visual experience without losing speed and technical depth.

## Position in the ecosystem

`app` is the GUI layer. All scan logic lives in [`catnet-io/engine`](https://github.com/catnet-io/engine);
this repository is a thin consumer of the engine's event API, exactly like the CLI and the TUI.

## Goals
- Provide a friendly cross-platform desktop experience (mobile planned).
- Reuse shared engine contracts.
- Keep UI concerns separate from scan domain logic.
- Evolve as the visual frontend of the ecosystem.

## Status
Transition phase. This repository is being repositioned from a standalone scanner app to the GUI
frontend of a multi-repository architecture.

## Development & Security (DevSecOps)
- **Branching Policy**: `develop` is the main collaboration branch; `main` only accepts signed, automated PRs from `develop` created by `github-actions[bot]`.
- **CI/CD**: Workflows validate builds, dependencies, and SAST on both `main` and `develop` branches.
- **Vulnerabilities**: see [SECURITY.md](SECURITY.md) — do not open public issues.

## Part of the CatNet ecosystem

| | Repository | Role |
|---|---|---|
| ⚙️ | [catnet-io/engine](https://github.com/catnet-io/engine) | Shared Go scanning engine |
| 💻 | [catnet-io/catnet](https://github.com/catnet-io/catnet) | CLI |
| 🖥️ | [catnet-io/app](https://github.com/catnet-io/app) | Desktop GUI |
| 📟 | [catnet-io/tui](https://github.com/catnet-io/tui) | Terminal UI |

## License

MIT — see [LICENSE](LICENSE).
