# Microtonic Script Writer Agent Package

This package contains reusable agent instructions for creating, adapting, and
debugging Microtonic scripts with this SDK as the source of truth.

For a practical workflow guide, see [`vibe-coding.md`](vibe-coding.md).

Use the canonical instruction files directly, or use one of the platform
wrappers:

- Codex: [`codex/SKILL.md`](codex/SKILL.md)
- Claude: [`claude/CLAUDE.md`](claude/CLAUDE.md)
- ChatGPT: [`chatgpt/instructions.md`](chatgpt/instructions.md)

The package is intentionally stored inside the SDK so agents can inspect the
same documentation, examples, schemas, resources, and validation tools that
script authors use.
