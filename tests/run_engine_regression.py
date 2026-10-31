#!/usr/bin/env python3
"""Run combat determinism fixtures against the built Ninja desktop engine."""
import json
import shlex
import subprocess
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
build = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else root / "build"
target = "redalert_mac" if sys.platform == "darwin" else "redalert_linux"
suite = "combat"
test_name = "combat_rng_test"
output = build / (suite + "-regression")
output.mkdir(exist_ok=True)
log_path = output / "test.log"
try:
    with log_path.open("w") as log:
        subprocess.run(["cmake", "--build", str(build), "--target", target, "-j", "8"],
                       check=True, stdout=log, stderr=log)
        entries = json.loads((build / "compile_commands.json").read_text())
        entry = next(e for e in entries if e["file"].endswith("/CODE/INIT.CPP"))
        compile_args = shlex.split(entry["command"])
        # Exercise the real private production methods without adding test APIs to the engine.
        compile_args.insert(1, "-fno-access-control")
        obj = str(output / (test_name + ".o"))
        compile_args[compile_args.index("-o") + 1] = obj
        compile_args[-1] = str(root / "tests" / (test_name + ".cpp"))
        subprocess.run(compile_args, cwd=entry["directory"], check=True, stdout=log, stderr=log)
        commands = subprocess.check_output(["ninja", "-t", "commands", target], cwd=build, text=True)
        link = shlex.split(commands.splitlines()[-1])
        if link[0] == ":":
            link = link[link.index("&&") + 1:]
        if "&&" in link:
            link = link[:link.index("&&")]
        main_obj = next(a for a in link if a.endswith("/PORT/MAC/src/main.cpp.o"))
        link[link.index(main_obj)] = obj
        binary = str(output / test_name)
        link[link.index("-o") + 1] = binary
        subprocess.run(link, cwd=build, check=True, stdout=log, stderr=log)
        subprocess.run([binary], cwd=root, check=True, stdout=log, stderr=log)
except subprocess.CalledProcessError:
    print(log_path.read_text()[-3000:], file=sys.stderr)
    raise SystemExit(1)
print(f"{suite.capitalize()} regression passed. Evidence: {log_path}")
