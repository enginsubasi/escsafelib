# AGENTS.md

Repository instructions for Codex and other assistants that read this file.
They apply throughout this repository.

## Read before working

1. [Shared workflow](docs/ai/WORKFLOW.md): how both assistants maintain the
   same project context.
2. [Current handoff](docs/ai/HANDOFF.md): the last recorded work, verification
   and remaining steps. Check it against the actual Git state.
3. [CLAUDE.md](CLAUDE.md): the existing project rules, module contracts,
   verification procedures and environment caveats. These apply to Codex too,
   despite the filename. Read the file itself, not just this pointer.
4. [codingReference.md](codingReference.md): the coding style authority.

Keep technical guidance in its existing source rather than copying it here.
This file is a short entry point, not a second project manual.

## Codex working notes

- Follow the language and commit conventions in `CLAUDE.md`.
- Its shell examples use Bash. Run them in Bash or translate them to the
  active shell; do not paste Bash syntax into PowerShell unchanged. Verify
  tool availability rather than assuming an earlier session's environment.
- Before finishing work that changes the repository, update the shared
  handoff as described in `docs/ai/WORKFLOW.md`. A future Claude session must
  be able to continue using repository files alone.

The entry point follows the official
[AGENTS.md guidance](https://learn.chatgpt.com/docs/agent-configuration/agents-md).
