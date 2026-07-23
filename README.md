# catnet-scanner

catnet-scanner is the cross-platform (desktop & mobile) application of the CatNet ecosystem.

It provides a modern graphical interface for network discovery and scan workflows, built for users who want a more accessible visual experience without losing speed and technical depth.

## Position in the ecosystem
catnet-scanner is the GUI layer of the CatNet ecosystem.
Core scanning logic is being progressively centralized in catnet-core.

## Goals
- Provide a friendly cross-platform (desktop & mobile) experience.
- Reuse shared engine contracts.
- Keep UI concerns separate from scan domain logic.
- Evolve as the visual frontend of the ecosystem.

## Status
Transition phase. This repository is being repositioned from standalone scanner app to GUI frontend in a multi-repository architecture.


## Development & Security (DevSecOps)
- **Branching Policy**: `develop` is the main collaboration branch; `main` only accepts signed, automated PRs from `develop` created by `github-actions[bot]`.
- **CI/CD**: Workflows validate builds, dependencies, and SAST on both `main` and `develop` branches.

## Part of the CatNet ecosystem

| | Repository | Role |
|---|---|---|
| ⚙️ | [catnet-io/engine](https://github.com/catnet-io/engine) | Shared Go scanning engine |
| 💻 | [catnet-io/catnet](https://github.com/catnet-io/catnet) | CLI |
| 📱 | [catnet-io/app](https://github.com/catnet-io/app) | Desktop & Mobile app |
| 📟 | [catnet-io/tui](https://github.com/catnet-io/tui) | Terminal UI |