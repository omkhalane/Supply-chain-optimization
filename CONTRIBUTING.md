# Contributing Guidelines

Thank you for your interest in contributing to Supply Chain Optimization.

## Development Setup

1. Fork and clone the repository.
2. Install frontend dependencies in [Frontend](Frontend).
3. Ensure `g++` is available for backend builds.
4. Run the project locally using [run.sh](run.sh).

## Branch Naming

Use clear branch names:

- `feature/<short-name>`
- `fix/<short-name>`
- `docs/<short-name>`

## Commit Messages

Prefer conventional style:

- `feat: add capacity-aware scoring`
- `fix: handle empty roads array`
- `docs: improve README setup`

## Pull Request Checklist

- Scope is focused and minimal.
- Code builds successfully.
- UI changes include screenshots (if applicable).
- Documentation is updated (README or related docs).
- No unrelated formatting-only changes.

## Code Style

- Keep existing style and naming conventions.
- Avoid introducing breaking changes without discussion.
- Add comments only where logic is non-obvious.

## Reporting Issues

Please include:

- Expected behavior
- Actual behavior
- Steps to reproduce
- Input case file used (if relevant)
- Environment details (OS, compiler/node version)
