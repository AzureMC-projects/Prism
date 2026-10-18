# Contributing

Thanks for helping improve Prism.

## Before opening a pull request

- Keep changes focused and explain the user-facing impact.
- Build with MSVC x64 and run CTest.
- Add tests for new API contracts and non-game-specific logic.
- Document ABI changes and update the API version when compatibility breaks.
- Do not commit game binaries, personal data, crash dumps, credentials, or third-party mods.

## Reports

Include Windows version, Prism commit/tag, architecture, steps to reproduce, expected behavior, actual behavior, and relevant sanitized logs. Never attach account tokens or personal files.

## Native mod security

Mods are arbitrary native code. Review and trust a mod before installing it. Prism does not claim to sandbox plugins.
