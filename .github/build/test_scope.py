#!/usr/bin/env python3
"""Select source UI regressions for source-only changes; otherwise keep full CI."""
import os
from pathlib import Path
import re
import subprocess

SOURCE_FILES = {
    "src/streams.c", "src/selospacote.c", "src/selospacote.h", "src/sourceperf.h",
    "src/fontenota.c", "src/stream_formatter.inc",
    "src/stream_formatter_draw.inc", "src/stream_formatter_emoji.inc", "src/source_backdrop.inc",
    ".github/workflows/playback-v17-ipk.yml", ".github/build/test_scope.py",
    "tests/test_scope.py", "tests/sourceperf.c", "tests/sourceperf_peer.c", "tests/sourceperf.sh",
    "tests/source_scroll.py", "tests/source_selection.py", "tests/source_badge_position.py",
    "tests/source_badge_readiness.py", "tests/source_badge_render.py", "tests/source_badge_gl.inc",
    "tests/selospacote.c", "tests/selospacote.sh", "tests/source_refresh_identity.py",
    "tests/source_refresh_identity.c", "tests/fontes_formatter.c",
    "tests/fontes_formatter.sh", "tests/source_audio_language.c",
    "tests/source_audio_language.sh", "tests/source_audio_cache.py", "tests/source_backdrop.py",
    "tests/source_backdrop_gl.inc", "tests/formatter_filename_tail.c",
    "tests/formatter_filename_tail.sh",
}


DIAGNOSTIC_FILES = {"src/app.c", "src/main.c"}


def choose(paths, force_full=False, verified_probes=()):
    allowed = SOURCE_FILES | (set(verified_probes) & DIAGNOSTIC_FILES)
    return "sources" if paths and not force_full and all(p in allowed for p in paths) else "full"


def without_scene_probes(text):
    text = re.sub(r'^#include "sourceperf.h"\n', '', text, flags=re.M)
    text = re.sub(r'^[ \t]*SOURCEPERF_SCENE_(?:BEGIN|END)\(NV_SRC_[A-Z_]+\);\n', '', text, flags=re.M)
    return re.sub(r' SOURCEPERF_SCENE_END\(NV_SRC_[A-Z_]+\);', '', text)


def without_source_backdrop(text):
    text = re.sub(r'^#include "source_backdrop.inc"\n', '', text, flags=re.M)
    return text.replace("fontes_fundo(podeParar, cat_revisao(), desenharAtrasDoPainel, &agora)",
                        "spainel_fundo(podeParar, cat_revisao(), desenharAtrasDoPainel, &agora)")


def diagnostic_change_safe(base, head, path):
    if path not in DIAGNOSTIC_FILES:
        return False
    try:
        texts = [subprocess.check_output(["git", "show", ref + ":" + path],
                                        text=True, stderr=subprocess.DEVNULL)
                 for ref in (base or head + "^", head)]
    except (OSError, subprocess.CalledProcessError):
        return False
    # app.c may also route this exact background callback through the tested
    # source-sheet wrapper. Every other app/main runtime edit keeps full CI.
    if path == "src/app.c":
        texts = list(map(without_source_backdrop, texts))
    return without_scene_probes(texts[0]) == without_scene_probes(texts[1])


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
    probes = [path for path in paths if diagnostic_change_safe(base, head, path)]
    scope = choose(paths, force, probes)
    print(f"[ci-tests] scope={scope}; changed_paths={len(paths)}; unknown or shared changes use full regressions")
    with Path(os.environ["GITHUB_OUTPUT"]).open("a", encoding="utf-8") as output:
        output.write(f"scope={scope}\nfull={'true' if scope == 'full' else 'false'}\n")
