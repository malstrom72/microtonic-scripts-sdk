# Microtonic Script Writer Agent Package

This package contains reusable agent instructions for creating, adapting, and
debugging Microtonic scripts with this SDK as the source of truth.

For a practical workflow guide, see [`vibe-coding.md`](vibe-coding.md).
For accumulated Cushy GUI details, gotchas, and implementation tips, see
[`cushy-notes.md`](cushy-notes.md).

Use [`instructions.md`](instructions.md) as the canonical instruction file.
The platform-specific files are thin wrappers for tools that expect a particular
entry-point format:

- Codex: [`codex/SKILL.md`](codex/SKILL.md)
- Claude: [`claude/CLAUDE.md`](claude/CLAUDE.md)
- ChatGPT: [`chatgpt/instructions.md`](chatgpt/instructions.md)

They all point back to the same shared instructions and should not be treated
as separate documentation sets.

The package is intentionally stored inside the SDK so agents can inspect the
same documentation, examples, schemas, resources, and validation tools that
script authors use.
