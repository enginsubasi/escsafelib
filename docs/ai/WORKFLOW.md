# Shared assistant workflow

Codex and Claude use the same repository files for durable context. Both
assistants must follow this workflow. Chat history and private assistant
memory may help, but must not be the only record of a project decision or
unfinished task.

## Where information belongs

Paths in this table are relative to the repository root. Keep each kind of
information in its owning file and link to it from elsewhere.

| File | Responsibility |
|---|---|
| [AGENTS.md](../../AGENTS.md) | Codex entry point and shell adaptation notes. |
| [CLAUDE.md](../../CLAUDE.md) | Claude entry point and the existing technical guidance shared by both assistants. |
| [codingReference.md](../../codingReference.md) | Coding and documentation style. |
| [README.md](../../README.md) | Public library overview and usage. |
| [tools/README.md](../../tools/README.md) | Verification tools and generator workflow. |
| [docs/standards/README.md](../standards/README.md) | Standards source register, application notes and evidence boundaries; not a compliance declaration. |
| [WORKFLOW.md](WORKFLOW.md) | Shared context maintenance and handoff rules. |
| [HANDOFF.md](HANDOFF.md) | Current work, decisions needed to resume, verification evidence and next steps. |
| [HISTORY.md](HISTORY.md) | Durable milestones, completed work, verification outcomes and findings that explain the current handoff. |

The current user request and the assistant's runtime instructions take
precedence over repository guidance. Within these files, use the owner
listed above for each topic. A handoff records facts and work status; it
does not override a coding rule or authorize a new task. If two records
disagree, inspect the relevant code, Git history and evidence, then correct
the stale record. Ask only when the intended decision cannot be established.

Existing review and verification checklists remain in
[safety-reviewer.md](../../.claude/agents/safety-reviewer.md) and
[verifier.md](../../.claude/agents/verifier.md). Consult them when relevant;
their presence does not install those roles in another assistant.

## At the start of a session

1. Read this workflow, `HANDOFF.md`, `CLAUDE.md` and `codingReference.md`.
   Read `HISTORY.md` when the handoff relies on prior decisions or findings.
   Read the relevant module and tool documentation before changing them.
2. Check `git status --short`, `git branch --show-current`,
   `git log -5 --oneline`, `git diff` and `git diff --cached`. Inspect relevant untracked
   files too; Git diff does not show their contents.
3. Compare the handoff's branch and base commit with this checkout. Treat a
   mismatch as a reason to reconcile context, not to reset the repository.
   Preserve existing changes from the owner or the other assistant.
4. Continue the user's requested work using the actual files. Do not assume
   an earlier assistant's test claim applies to a later revision.

## While working

- Update the owning documentation alongside the change that makes it stale.
  Technical rules stay in `CLAUDE.md`, style in `codingReference.md`, and
  synchronization rules here. Keep both entry points linked to these files;
  their contents do not need to be identical.
- Record decisions that the next session needs, including a brief reason.
  Promote durable technical decisions into the relevant existing document;
  do not leave them only in the handoff or private memory.
- Keep the existing preference for agreeing designs in chat and implementing
  directly. These coordination files are not a request for additional
  specifications or implementation plans.
- Record verification accurately: command, scope, result and relevant
  limitation. Distinguish checks run now from inherited historical results.
  For documentation-only changes, check links and the diff; do not describe
  the library as tested when no library tests were run.
- At a meaningful checkpoint in long work, save enough state in `HANDOFF.md`
  to recover from an interrupted session. Do not wait for the final reply if
  important context could otherwise be lost.

## Before handing work back

After repository changes, update `HANDOFF.md` before the final response.
Also update it for a substantive finding that affects ongoing work, unless
the user requested a read-only review. A question with no new project state
does not need a bookkeeping edit.

Keep the handoff concise and current, using these fields:

- Last updated date (`YYYY-MM-DD`), assistant, branch and base commit. The
  base commit is the revision the work started from, not a claim that the
  changes have been committed.
- Objective and status: in progress, completed, or blocked with a reason.
- Changes and affected paths, including any relevant uncommitted work.
- Decisions and reasons needed to continue.
- Verification: commands and outcomes, failures and skipped checks with why.
- Remaining work, open questions and the next concrete step, or explicitly
  none. Completion of the present task is not a claim that the whole library
  has no outstanding work.

Replace obsolete task details instead of appending an endless session log.
Retain unresolved items until they are resolved or the user drops them.
Use `HISTORY.md` for concise durable milestones; do not include secrets or
conversation transcripts. State commit and push status accurately; updating a
handoff does not itself require a commit or push.

## Switching assistants or working concurrently

In the same working directory, saved files carry the context, including
uncommitted changes. In another clone, branch or worktree, the relevant code
and documentation changes must first be transferred through the usual Git
workflow. Do not claim that another checkout has received local edits.

When work is concurrent, prefer separate branches/worktrees and avoid having
two assistants edit the same files at once. Re-read the handoff and diff
before writing; preserve another session's new entries. On integration,
reconcile both handoffs against the combined code and checks rather than
blindly keeping the last writer's version.

This is a file-based handoff protocol, not a background synchronization
service. Each assistant is responsible for reading and updating it. If an
already-open session has not loaded the new instructions, explicitly read
the entry point and shared files before continuing.
