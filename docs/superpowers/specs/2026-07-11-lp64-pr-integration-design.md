# LP64 Pull Request Integration Design

## Goal

Integrate the verified fixes from GitHub PRs #2 and #5 into one tested maintainer-owned change, preserve contributor attribution, merge the integration through GitHub, and close the superseded contributor PRs with clear references to the merged work.

## Scope

The integration includes:

- Watcom-width normalization for trigger action IDs, event IDs, action data, event data, and trigger house ownership from PR #5.
- WSA on-disk header layout and LP64 delta-buffer sizing from PR #2.
- Non-throwing fixed-pool allocation declarations from PR #2.
- EVA speech bounds checks from PR #2.
- The path-following trailing-terminator capacity fix from PR #2.
- The sidebar palette indexing fix from PR #2.
- Cell adjacency, sidebar build-list, and malformed save-description bounds fixes from PR #2.

The incomplete trigger-data commit from PR #2 is excluded because PR #5 supersedes it. Unrelated refactoring is out of scope.

## Integration Strategy

Work begins from the current `main` commit in an isolated worktree on a maintainer-owned branch. Regression tests are written and observed failing before production changes are integrated.

Valid contributor commits are cherry-picked so their original authors remain recorded in Git history. PR #5 is integrated in full. PR #2 is integrated commit-by-commit, omitting its superseded trigger-normalization commit. Any conflict in `CODE/TACTION.CPP` is resolved in favor of PR #5's complete trigger-width behavior while retaining non-overlapping safety guards.

Maintainer follow-up commits add or adjust tests and make only the smallest changes required to produce a coherent, passing integration.

## Code Structure

Trigger-width conversion should be exposed through a small C++98-compatible helper used by both action and event parsing. The helper preserves the original Watcom semantics:

- one-byte enum-backed data narrows through a signed 8-bit value;
- sound identifiers narrow through a signed 16-bit value;
- numeric, time, waypoint, team, trigger, and other wide values remain unchanged.

`TActionClass`, `TEventClass`, and `TriggerTypeClass` use the same helper so parsing rules cannot drift between files. Existing source organization and naming conventions remain otherwise unchanged.

The save-description bounds logic may be extracted into a small testable helper if direct testing of `Get_Savefile_Info` would require linking the entire game. No broader save-system redesign is included.

## Testing

The permanent regression coverage includes:

1. Trigger-width tests covering ordinary and sign-extended action IDs, event IDs, speech values, and house values. These must fail on the current `main` behavior and pass after integration.
2. WSA tests updated for a 14-byte header and the corresponding resident offsets (`0x12` and `0x42` for the existing fixture).
3. A C++98 allocation test proving that a null-returning class-specific `operator new(size_t) throw()` does not invoke the constructor.
4. A malformed save-description test proving a full non-NUL-terminated description is handled without an out-of-bounds scan or trim underflow.
5. Focused source-level contracts for the path terminator slot, palette entry indexing, sidebar capacity comparison, and map-cell upper bound where constructing full game state would make a unit test disproportionate.

Verification requires:

- `tests/run_script_tests.sh` passes;
- a clean CMake/Ninja build of `redalert_mac` passes;
- focused regression binaries pass;
- `git diff --check` passes;
- the maintainer integration branch contains no unrelated changes.

## Publishing and Attribution

After local verification, push the maintainer branch and open a GitHub pull request that references PRs #2 and #5 and names both contributors. The pull request description lists which commits were integrated, which PR #2 commit was superseded, and the verification evidence.

Merge the maintainer pull request only after GitHub reports it mergeable and the local verification remains current. Then close PRs #2 and #5 with comments linking the merged integration and explicitly thanking each contributor. Do not imply their work was rejected; explain that it was consolidated to resolve overlap and add tests.

## Failure Handling

If an integrated fix fails its focused regression or the full build, stop and isolate that commit rather than weakening the test. If GitHub state changes before publishing, fetch the new state and re-evaluate the affected PR before closing it. If branch protection prevents merging, leave the integration PR open and report the exact required check or approval instead of bypassing protection.
