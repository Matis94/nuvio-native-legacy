#!/usr/bin/env python3
"""CI suite selection must fail open to full testing for shared/unknown changes."""
import importlib.util
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("test_scope", ROOT / ".github/build/test_scope.py")
scope = importlib.util.module_from_spec(spec)
spec.loader.exec_module(scope)

assert scope.choose(["src/streams.c", "src/sourceperf.h", "tests/sourceperf.c",
                     ".github/workflows/playback-v17-ipk.yml"]) == "sources"
for path in ["src/video.c", "src/app.c", "src/streams.h", "src/rede.c", "src/dts/dts_engine.c",
             "src/ajustes.c", "src/tex_cache.c", "src/fontepref.c", "tools/Dockerfile", "unknown"]:
    assert scope.choose(["src/streams.c", path]) == "full", path
assert scope.choose([]) == "full"
assert scope.choose(["src/streams.c"], True) == "full"
workflow = (ROOT / ".github/workflows/playback-v17-ipk.yml").read_text()
assert scope.build_signature(workflow) == scope.build_signature(workflow.replace(" -DNV_SOURCE_PERF_DIAG", ""))
assert scope.build_signature(workflow) != scope.build_signature(workflow.replace("-DNV_DV7_MAPPING_TEST", ""))
assert scope.build_signature(workflow) != scope.build_signature(workflow.replace("NUVIO_DTS_FFMPEG: '1'", "NUVIO_DTS_FFMPEG: '0'"))
assert scope.changed("0" * 40, "1" * 40) == []
assert scope.changed("--bad-base", "1" * 40) == []
assert scope.changed("1" * 40, "--bad-head") == []
with tempfile.TemporaryDirectory(prefix="nuvio-test-scope-git-") as directory:
    path = Path(directory)
    subprocess.run(["git", "init", "-q", directory], check=True)
    (path / "src").mkdir()
    source = path / "src/streams.c"
    source.write_text("initial\n")
    def commit():
        subprocess.run(["git", "add", "."], cwd=path, check=True)
        subprocess.run(["git", "-c", "user.name=CI fixture", "-c", "user.email=fixture@example.invalid",
                        "commit", "-qm", "fixture"], cwd=path, check=True)
        return subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=path, text=True).strip()
    base = commit()
    source.write_text("updated\n")
    head = commit()
    previous = Path.cwd()
    try:
        os.chdir(path)
        assert scope.changed(base, head) == ["src/streams.c"]
        assert scope.choose(scope.changed("", head)) == "sources"
        source.rename(path / "src/rede.c")
        renamed = commit()
        assert scope.choose(scope.changed(base, renamed)) == "full"
    finally:
        os.chdir(previous)
with tempfile.TemporaryDirectory(prefix="nuvio-test-scope-") as directory:
    output = Path(directory) / "result"
    env = {**os.environ, "NUVIO_TEST_HEAD": "1" * 40, "NUVIO_TEST_BASE": "2" * 40,
           "GITHUB_OUTPUT": str(output)}
    subprocess.run(["python3", str(ROOT / ".github/build/test_scope.py")], env=env, check=True, cwd=directory)
    assert output.read_text() == "scope=full\nfull=true\n"
print("PASS: source-only CI selection, shared dependency fallback, explicit full run and unavailable-history fallback")
