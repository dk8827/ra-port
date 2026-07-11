# LP64 Pull Request Integration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Integrate the verified fixes from PRs #2 and #5 into one tested maintainer-owned branch, preserve contributor authorship, merge it, and close the superseded PRs.

**Architecture:** Keep the original source layout, but centralize Watcom-width conversion in one C++98 inline helper shared by trigger actions and events. Integrate contributor commits individually, excluding PR #2's superseded trigger commit, and add lightweight unit tests plus source contracts to the existing `tests/run_script_tests.sh` harness.

**Tech Stack:** C++98, Bash, CMake 3.22+, Ninja, SDL2, Git, GitHub CLI.

## Global Constraints

- Preserve original contributor authorship by cherry-picking their functional commits.
- Exclude PR #2 commit `0a77700`; PR #5 supersedes it.
- Do not perform unrelated refactoring.
- Every production change must be preceded by a failing regression or source-contract test.
- Verification requires `tests/run_script_tests.sh`, a clean `redalert_mac` build, focused tests, and `git diff --check`.
- Close PRs #2 and #5 only after the maintainer integration PR is merged.

---

### Task 1: Create the isolated integration workspace

**Files:**
- Modify: `.gitignore`

**Interfaces:**
- Consumes: committed design and plan on `spec/lp64-pr-integration`
- Produces: isolated branch `fix/lp64-pr-integration` containing the documentation commits

- [ ] **Step 1: Ignore the project-local worktree directory**

Add this line to `.gitignore`:

```gitignore
.worktrees/
```

- [ ] **Step 2: Commit the worktree ignore rule**

```bash
git add .gitignore
git commit -m "chore: ignore local worktrees"
```

- [ ] **Step 3: Verify the ignore rule and create the isolated branch**

```bash
git check-ignore -q .worktrees
git worktree add .worktrees/lp64-pr-integration -b fix/lp64-pr-integration spec/lp64-pr-integration
```

Expected: `.worktrees/lp64-pr-integration` is on `fix/lp64-pr-integration` and includes the design, plan, and `.gitignore` commits.

- [ ] **Step 4: Return the primary checkout to `main`**

```bash
git switch main
```

Expected: the primary checkout is clean on `main`; all implementation work occurs in `.worktrees/lp64-pr-integration`.

- [ ] **Step 5: Fetch and pin contributor heads**

```bash
git fetch origin pull/2/head:refs/remotes/origin/pr-2 pull/5/head:refs/remotes/origin/pr-5
git rev-parse origin/pr-2 origin/pr-5
```

Expected SHAs:

```text
3bdd71a9f89fce0eb5c10264aa10facb6aa1b2b4
30f6885f91d6a535c78119f8387553bb0def9315
```

- [ ] **Step 6: Verify the baseline**

```bash
tests/run_script_tests.sh
cmake -S . -B build -G Ninja
cmake --build build --target redalert_mac -j 8
```

Expected: source tests pass and `redalert_mac` links successfully.

---

### Task 2: Integrate complete trigger-width normalization

**Files:**
- Create: `CODE/TRIGGERWIDTH.H`
- Create: `tests/trigger_width_test.cpp`
- Modify: `CODE/TACTION.CPP`
- Modify: `CODE/TEVENT.CPP`
- Modify: `CODE/TRIGTYPE.CPP`
- Modify: `tests/run_script_tests.sh`

**Interfaces:**
- Produces: `Legacy_Trigger_Byte(long)`, `Legacy_Trigger_Word(long)`, and `Normalize_Trigger_Data(NeedType, long)`
- Consumers: `TActionClass::Read_INI`, `TActionClass::operator()`, `TEventClass::Read_INI`, `TEventClass::operator()`, and `TriggerTypeClass::Fill_In`

- [ ] **Step 1: Add the failing lightweight trigger-width test**

Create `tests/trigger_width_test.cpp`:

```cpp
#include "DEFINES.H"
#include "TRIGGERWIDTH.H"

#include <assert.h>

int main(void)
{
	assert(Legacy_Trigger_Byte(-235) == 21);
	assert(Legacy_Trigger_Byte(-246) == 10);
	assert(Legacy_Trigger_Byte(-247) == 9);
	assert(Normalize_Trigger_Data(NEED_SPEECH, -191) == 65);
	assert(Normalize_Trigger_Data(NEED_HOUSE, -247) == 9);
	assert(Normalize_Trigger_Data(NEED_SOUND, -65437) == 99);
	assert(Normalize_Trigger_Data(NEED_NUMBER, 500) == 500);
	return 0;
}
```

Append this compile/run block to `tests/run_script_tests.sh`:

```bash
"${CXX:-c++}" -std=gnu++98 -DTRUE_FALSE_DEFINED -I"$ROOT_DIR/CODE" \
  "$ROOT_DIR/tests/trigger_width_test.cpp" -o "$tmpdir/trigger_width_test"
"$tmpdir/trigger_width_test"
```

- [ ] **Step 2: Verify the test fails for the missing helper**

```bash
tests/run_script_tests.sh
```

Expected: compilation fails because `TRIGGERWIDTH.H` does not exist.

- [ ] **Step 3: Commit the failing regression test**

```bash
git add tests/trigger_width_test.cpp tests/run_script_tests.sh
git commit -m "test: reproduce legacy trigger width parsing"
```

- [ ] **Step 4: Cherry-pick PR #5 with original authorship**

```bash
git cherry-pick 12c5c852 30f6885f
```

Expected: `CODE/TACTION.CPP`, `CODE/TEVENT.CPP`, and `CODE/TRIGTYPE.CPP` contain the complete PR #5 behavior.

- [ ] **Step 5: Add the shared C++98 helper**

Create `CODE/TRIGGERWIDTH.H`:

```cpp
#ifndef TRIGGERWIDTH_H
#define TRIGGERWIDTH_H

#include <stdint.h>

static inline long Legacy_Trigger_Byte(long value)
{
	return (long)(int8_t)value;
}

static inline long Legacy_Trigger_Word(long value)
{
	return (long)(int16_t)value;
}

static inline long Normalize_Trigger_Data(NeedType need, long value)
{
	switch (need) {
		case NEED_THEME:
		case NEED_MOVIE:
		case NEED_SPEECH:
		case NEED_HOUSE:
		case NEED_SPECIAL:
		case NEED_QUARRY:
		case NEED_BOOL:
			return Legacy_Trigger_Byte(value);

		case NEED_SOUND:
			return Legacy_Trigger_Word(value);

		default:
			return value;
	}
}

#endif
```

In `CODE/TACTION.CPP` and `CODE/TEVENT.CPP`, include `TRIGGERWIDTH.H`, delete the duplicated static normalization functions, and replace signed casts with `Legacy_Trigger_Byte(...)` and data normalization with `Normalize_Trigger_Data(Action_Needs(Action), Data.Value)` or `Normalize_Trigger_Data(Event_Needs(Event), Data.Value)`.

In `CODE/TRIGTYPE.CPP`, include `TRIGGERWIDTH.H` and parse `House` through `Legacy_Trigger_Byte(...)`.

- [ ] **Step 6: Add source contracts for the wiring**

Add to `tests/run_script_tests.sh`:

```bash
assert_file_contains CODE/TACTION.CPP "Normalize_Trigger_Data(Action_Needs(Action), Data.Value)"
assert_file_contains CODE/TEVENT.CPP "Normalize_Trigger_Data(Event_Needs(Event), Data.Value)"
assert_file_contains CODE/TRIGTYPE.CPP "Legacy_Trigger_Byte(atoi(strtok(NULL, \",\")))"
```

- [ ] **Step 7: Verify trigger tests and the game build**

```bash
tests/run_script_tests.sh
cmake --build build --target redalert_mac -j 8
```

Expected: all tests pass and the game links.

- [ ] **Step 8: Commit the shared trigger implementation**

```bash
git add CODE/TRIGGERWIDTH.H CODE/TACTION.CPP CODE/TEVENT.CPP CODE/TRIGTYPE.CPP tests/run_script_tests.sh
git commit -m "refactor: centralize legacy trigger width handling"
```

---

### Task 3: Integrate and test the WSA LP64 layout fix

**Files:**
- Modify: `tests/wsa_file_format_test.cpp`
- Modify through cherry-pick: `WIN32LIB/WSA/WSA.CPP`
- Modify through cherry-pick: `WIN32LIB/WSA/wsa_file_format.h`

**Interfaces:**
- Produces: 14-byte WSA header interpretation and correctly sized LP64 delta storage
- Consumes: existing `WSA_Read_File_Offset` and `WSA_Resident_Frame_Offset` test fixture

- [ ] **Step 1: Change WSA expectations before production code**

Update `tests/wsa_file_format_test.cpp`:

```cpp
assert(WSA_FILE_HEADER_SIZE == 14);
assert(WSA_Resident_Frame_Offset((char const *)offsets, 1) == 0x12);
assert(WSA_Resident_Frame_Offset((char const *)offsets, 2) == 0x42);
```

- [ ] **Step 2: Verify the WSA test fails on the 16-byte implementation**

```bash
tests/run_script_tests.sh
```

Expected: `wsa_file_format_test` aborts because `WSA_FILE_HEADER_SIZE` is 16.

- [ ] **Step 3: Commit the failing WSA regression**

```bash
git add tests/wsa_file_format_test.cpp
git commit -m "test: expect the packed WSA disk header"
```

- [ ] **Step 4: Cherry-pick the WSA fix**

```bash
git cherry-pick 136fdc3a
```

- [ ] **Step 5: Verify WSA behavior and the full build**

```bash
tests/run_script_tests.sh
cmake --build build --target redalert_mac -j 8
```

Expected: WSA tests pass and the game links.

---

### Task 4: Integrate and test null-safe fixed-pool allocation

**Files:**
- Create: `tests/operator_new_null_test.cpp`
- Modify: `tests/run_script_tests.sh`
- Modify through cherry-pick: fixed-pool class declarations and definitions under `CODE/`

**Interfaces:**
- Produces: class-specific `operator new(size_t) throw()` declarations/definitions for fixed pools
- Consumes: Clang C++98 empty exception specification semantics

- [ ] **Step 1: Add the compiler-semantics unit test**

Create `tests/operator_new_null_test.cpp`:

```cpp
#include <stddef.h>
#include <stdio.h>

static int constructor_calls = 0;

class NullPoolObject {
public:
	NullPoolObject() { constructor_calls++; }
	static void *operator new(size_t) throw() { return NULL; }
};

int main(void)
{
	NullPoolObject *object = new NullPoolObject;
	if (object != NULL || constructor_calls != 0) {
		fprintf(stderr, "FAIL: null pool allocation invoked construction\n");
		return 1;
	}
	return 0;
}
```

Add to `tests/run_script_tests.sh`:

```bash
perl -0ne 'exit(/class BuildingClass[\s\S]*operator new\s*\(\s*size_t size\s*\)\s*throw\s*\(\s*\)/s ? 0 : 1)' "$ROOT_DIR/CODE/BUILDING.H" \
  || fail "BuildingClass pool allocation must be non-throwing"

"${CXX:-c++}" -std=gnu++98 "$ROOT_DIR/tests/operator_new_null_test.cpp" -o "$tmpdir/operator_new_null_test"
"$tmpdir/operator_new_null_test"
```

- [ ] **Step 2: Verify the production source contract fails**

```bash
tests/run_script_tests.sh
```

Expected: failure stating `BuildingClass pool allocation must be non-throwing`.

- [ ] **Step 3: Commit the allocation regression test**

```bash
git add tests/operator_new_null_test.cpp tests/run_script_tests.sh
git commit -m "test: cover null-returning pool allocation semantics"
```

- [ ] **Step 4: Cherry-pick the fixed-pool declarations**

```bash
git cherry-pick 049b1080
```

- [ ] **Step 5: Verify source contracts, semantics, and build**

```bash
tests/run_script_tests.sh
cmake --build build --target redalert_mac -j 8
```

Expected: allocation tests pass and the game links.

---

### Task 5: Integrate bounds and memory-safety fixes

**Files:**
- Create: `CODE/SAVEDESCRIPTION.H`
- Create: `tests/save_description_test.cpp`
- Modify: `CODE/SAVELOAD.CPP`
- Modify: `tests/run_script_tests.sh`
- Modify through cherry-pick: `CODE/AUDIO.CPP`, `CODE/FINDPATH.CPP`, `CODE/SIDEBAR.CPP`, `CODE/CELL.CPP`

**Interfaces:**
- Produces: `Normalize_Save_Description(char *, size_t)`
- Consumers: `Get_Savefile_Info`

- [ ] **Step 1: Add failing safety contracts and save test**

Create `tests/save_description_test.cpp`:

```cpp
#include "SAVEDESCRIPTION.H"

#include <assert.h>
#include <string.h>

int main(void)
{
	char normal[16] = "Mission\r\n";
	Normalize_Save_Description(normal, sizeof(normal));
	assert(strcmp(normal, "Mission") == 0);

	char full[44];
	memset(full, 'A', sizeof(full));
	Normalize_Save_Description(full, sizeof(full));
	assert(full[43] == '\0');
	assert(strlen(full) == 43);

	char short_value[2] = "A";
	Normalize_Save_Description(short_value, sizeof(short_value));
	assert(strcmp(short_value, "A") == 0);
	return 0;
}
```

Add these contracts and compile block to `tests/run_script_tests.sh`:

```bash
assert_file_contains CODE/AUDIO.CPP "if (voice < VOX_FIRST || voice >= VOX_COUNT) return;"
assert_file_contains CODE/FINDPATH.CPP "while (path->Length < max_cells - 1)"
assert_file_contains CODE/SIDEBAR.CPP "memset(&pal[CYCLE_COLOR_START], 0x3f, CYCLE_COLOR_COUNT * sizeof(RGBClass))"
assert_file_contains CODE/SIDEBAR.CPP "if (BuildableCount < MAX_BUILDABLES)"
assert_file_contains CODE/CELL.CPP ">= MAP_CELL_TOTAL"
assert_file_contains CODE/SAVELOAD.CPP "Normalize_Save_Description(descr_buf, DESCRIP_MAX)"

"${CXX:-c++}" -std=gnu++98 -I"$ROOT_DIR/CODE" \
  "$ROOT_DIR/tests/save_description_test.cpp" -o "$tmpdir/save_description_test"
"$tmpdir/save_description_test"
```

- [ ] **Step 2: Verify the contracts fail before integration**

```bash
tests/run_script_tests.sh
```

Expected: the first missing safety contract or `SAVEDESCRIPTION.H` fails.

- [ ] **Step 3: Commit the failing safety regressions**

```bash
git add tests/save_description_test.cpp tests/run_script_tests.sh
git commit -m "test: cover LP64 bounds and save descriptions"
```

- [ ] **Step 4: Cherry-pick the contributor safety fixes**

```bash
git cherry-pick 3086c3cd 47180b25 70c3d038 a5ae9cd0
```

Do not cherry-pick `0a77700` or the comment-only `3bdd71a` commit.

- [ ] **Step 5: Add the testable save-description helper**

Create `CODE/SAVEDESCRIPTION.H`:

```cpp
#ifndef SAVEDESCRIPTION_H
#define SAVEDESCRIPTION_H

#include <stddef.h>
#include <string.h>

static inline void Normalize_Save_Description(char *description, size_t capacity)
{
	if (description == NULL || capacity == 0) return;

	description[capacity - 1] = '\0';
	size_t length = strlen(description);
	if (length >= 2 && description[length - 2] == '\r' && description[length - 1] == '\n') {
		description[length - 2] = '\0';
	}
}

#endif
```

Include `SAVEDESCRIPTION.H` in `CODE/SAVELOAD.CPP` and replace its inline termination/trim block with:

```cpp
Normalize_Save_Description(descr_buf, DESCRIP_MAX);
```

- [ ] **Step 6: Verify all safety tests and build**

```bash
tests/run_script_tests.sh
cmake --build build --target redalert_mac -j 8
```

Expected: all safety contracts and unit tests pass; the game links.

- [ ] **Step 7: Commit the testable save handling**

```bash
git add CODE/SAVEDESCRIPTION.H CODE/SAVELOAD.CPP tests/run_script_tests.sh
git commit -m "refactor: make save description normalization testable"
```

---

### Task 6: Final verification, publish, merge, and close superseded PRs

**Files:**
- Verify all changed files
- No new production files

**Interfaces:**
- Produces: merged GitHub integration PR and closed PRs #2/#5

- [ ] **Step 1: Verify contributor heads have not changed**

```bash
test "$(gh pr view 2 --repo dk8827/ra-port --json headRefOid --jq .headRefOid)" = "3bdd71a9f89fce0eb5c10264aa10facb6aa1b2b4"
test "$(gh pr view 5 --repo dk8827/ra-port --json headRefOid --jq .headRefOid)" = "30f6885f91d6a535c78119f8387553bb0def9315"
```

- [ ] **Step 2: Run fresh complete verification**

```bash
rm -rf build-final
cmake -S . -B build-final -G Ninja
cmake --build build-final --target redalert_mac -j 8
tests/run_script_tests.sh
git diff --check main...HEAD
git status --short
```

Expected: clean build and tests pass, diff check is empty, and status is clean.

- [ ] **Step 3: Review integration scope**

```bash
git log --oneline --reverse main..HEAD
git diff --stat main...HEAD
git diff --name-status main...HEAD
```

Expected: only the design/plan, ignore rule, contributor fixes, helpers, and tests are present.

- [ ] **Step 4: Push and open the maintainer integration PR**

```bash
git push -u origin fix/lp64-pr-integration
gh pr create --repo dk8827/ra-port --base main --head fix/lp64-pr-integration \
  --title "Integrate LP64 trigger, WSA, pool, and bounds fixes" \
  --body-file /tmp/ra-port-integration-pr.md
```

The PR body must reference `#2` and `#5`, credit `hereiszee` and `xzeror`, list the omitted superseded commit `0a77700`, and include the exact verification commands.

- [ ] **Step 5: Confirm mergeability and merge**

```bash
integration_pr=$(gh pr view --repo dk8827/ra-port --json number --jq .number)
gh pr view "$integration_pr" --repo dk8827/ra-port --json state,mergeable,statusCheckRollup
gh pr merge "$integration_pr" --repo dk8827/ra-port --merge --delete-branch
```

Expected: the integration PR is merged into `main`.

- [ ] **Step 6: Comment on and close PR #5**

```bash
gh pr comment 5 --repo dk8827/ra-port --body "Thank you, @xzeror. Your complete Watcom-width trigger fix was integrated with regression coverage in #${integration_pr} and is now merged. I am closing this PR only because the work landed through the consolidated maintainer branch."
gh pr close 5 --repo dk8827/ra-port
```

- [ ] **Step 7: Comment on and close PR #2**

```bash
gh pr comment 2 --repo dk8827/ra-port --body "Thank you, @hereiszee. The WSA, fixed-pool allocation, EVA bounds, pathfinding, palette, cell, sidebar, and save-description fixes from this branch were integrated with updated regression coverage in #${integration_pr} and are now merged. The overlapping trigger-data commit was superseded by the more complete trigger fix from #5. I am closing this PR because the work landed through the consolidated maintainer branch."
gh pr close 2 --repo dk8827/ra-port
```

- [ ] **Step 8: Synchronize and verify final GitHub state**

```bash
git -C /Users/dk/Documents/ra-port switch main
git -C /Users/dk/Documents/ra-port pull --ff-only
gh pr list --repo dk8827/ra-port --state open
git -C /Users/dk/Documents/ra-port status --short
```

Expected: local `main` includes the merged integration, PRs #2 and #5 are closed, and the primary checkout is clean.
