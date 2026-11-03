#!/usr/bin/env python3
"""Select source UI regressions for source-only changes; otherwise keep full CI."""
import os
from pathlib import Path
import re
import subprocess

SOURCE_FILES = {
    "src/streams.c", "src/sourceperf.h", "src/stream_formatter.inc",
    "src/stream_formatter_draw.inc", "src/stream_formatter_emoji.inc",
    ".github/workflows/playback-v17-ipk.yml", ".github/build/test_scope.py",
    "tests/test_scope.py", "tests/sourceperf.c", "tests/sourceperf.sh",
    "tests/source_scroll.py", "tests/source_badge_position.py",
    "tests/source_badge_readiness.py", "tests/source_refresh_identity.py",
    "tests/source_refresh_identity.c", "tests/fontes_formatter.c",
    "tests/fontes_formatter.sh", "tests/source_audio_language.c",
    "tests/source_audio_language.sh", "tests/formatter_filename_tail.c",
    "tests/formatter_filename_tail.sh",
}


def choose(paths, force_full=False):
    return "sources" if paths and not force_full and all(p in SOURCE_FILES for p in paths) else "full"


def build_signature(text):
    values = re.findall(r"^      (NUVIO_[A-Z0-9_]+): (.*)$", text, re.M)
    return tuple((key, " ".join(value.replace("-DNV_SOURCE_PERF_DIAG", "").split()))
                 for key, value in values)


def build_configuration_safe(base, head):
    try:
        texts = [subprocess.check_output(["git", "show", ref + ":.github/workflows/playback-v17-ipk.yml"],
                                        text=True, stderr=subprocess.DEVNULL)
                 for ref in (base or head + "^", head)]
    except (OSError, subprocess.CalledProcessError):
        return False
    before, after = map(build_signature, texts)
    return bool(before and after and before == after)


def changed(base, head):
    if not re.fullmatch(r"[0-9a-fA-F]{40}", head or ""):
        return []
    if base and not re.fullmatch(r"[0-9a-fA-F]{40}", base):
        return []
    if base == "0" * 40:
        return []
    # Dispatch defaults to the last commit; an explicit full run always wins.
    base = base or head + "^"
    try:
        result = subprocess.run(["git", "diff", "--no-renames", "--name-only", "-z", base, head],
                                check=True, capture_output=True)
    except (OSError, subprocess.CalledProcessError):
        return []  # Missing shallow history or unknown base: never skip suites.
    return result.stdout.decode("utf-8", errors="replace").strip("\0").split("\0") if result.stdout else []


if __name__ == "__main__":
    base, head = os.environ.get("NUVIO_TEST_BASE", ""), os.environ.get("NUVIO_TEST_HEAD", "")
    paths = changed(base, head)
    force = os.environ.get("NUVIO_FULL_TESTS", "").lower() == "true"
    if ".github/workflows/playback-v17-ipk.yml" in paths and not build_configuration_safe(base, head):
        force = True
    scope = choose(paths, force)
    print(f"[ci-tests] scope={scope}; changed_paths={len(paths)}; unknown or shared changes use full regressions")
    with Path(os.environ["GITHUB_OUTPUT"]).open("a", encoding="utf-8") as output:
        output.write(f"scope={scope}\nfull={'true' if scope == 'full' else 'false'}\n")
