"""Verify that diagnostic code preserves official 2.0.8 settings and DTS logic."""
from pathlib import Path
import re
import subprocess

base = "87c8b8909f0d605f1d8351147fe5afb9de233b8d"
def original(path):
    return subprocess.check_output(["git", "show", base + ":" + path], text=True)
for name in ("src/ajustes.c", "src/ajustes.h"):
    assert Path(name).read_text() == original(name), name
controller = re.sub(r"#ifdef NV_HTTP_BUFFER_DIAG\n.*?#endif\n", "",
                    Path("src/dts/dts_playback.c").read_text(), flags=re.S)
assert " ".join(controller.split()) == " ".join(original("src/dts/dts_playback.c").split())
def open_config(text):
    start = text.index("int dts_engine_open(")
    return text[start:text.index("\n}", start) + 2]
assert open_config(Path("src/dts/dts_engine.c").read_text()) == open_config(original("src/dts/dts_engine.c"))
print("Official 2.0.8 settings, DTS controller and buffer configuration unchanged; diagnostics are test-only.")
