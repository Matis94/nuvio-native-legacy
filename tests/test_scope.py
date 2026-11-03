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
assert scope.choose(["src/fontenota.c", "tests/source_selection.py"]) == "sources"
assert scope.choose(["src/stream_formatter_draw.inc", "tests/source_formatter_motion.py"]) == "sources"
assert scope.choose(["src/source_backdrop.inc", "tests/source_backdrop_gl.inc", "tests/source_audio_cache.py"]) == "sources"
assert scope.choose(["src/selospacote.c", "src/selospacote.h", "tests/source_badge_render.py",
                     "tests/source_badge_gl.inc", "tests/selospacote.c", "src/regexjs.c", "src/regexjs.h",
                     "tests/regexjs_badge.c", "tests/regexjs_badge.sh"]) == "sources"
for path in ["src/video.c", "src/app.c", "src/streams.h", "src/rede.c", "src/dts/dts_engine.c",
             "src/ajustes.c", "src/tex_cache.c", "src/fontepref.c", "tools/Dockerfile", "unknown"]:
    assert scope.choose(["src/streams.c", path]) == "full", path
assert scope.choose([]) == "full"
assert scope.choose(["src/streams.c"], True) == "full"
assert scope.choose(["src/app.c"], verified_probes=["src/app.c"]) == "sources"
assert scope.choose(["src/video.c"], verified_probes=["src/video.c"]) == "full"
assert scope.without_scene_probes('call();\n') == scope.without_scene_probes(
    '#include "sourceperf.h"\nSOURCEPERF_SCENE_BEGIN(NV_SRC_APP_DRAW);\ncall();\nSOURCEPERF_SCENE_END(NV_SRC_APP_DRAW);\n')
swap = 'SDL_GL_SwapWindow(win);\nwait();\n'
swap_probes = ('SOURCEPERF_SWAP_BEGIN();\nSDL_GL_SwapWindow(win);\n'
               'SOURCEPERF_SWAP_END(playerUi, player_aberto(), player_carregando());\nwait();\n')
assert scope.without_scene_probes(swap_probes) == swap
assert scope.without_scene_probes(swap_probes.replace('playerUi,', 'changed(),')) != swap
assert scope.without_scene_probes(swap_probes.replace('wait();', 'otherWait();')) != swap
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
        app = path / "src/app.c"
        app.write_text("void draw(void) {\n  drawBackground(); }\n")
        before_probe = commit()
        app.write_text('#include "sourceperf.h"\nvoid draw(void) {\n'
                       '  SOURCEPERF_SCENE_BEGIN(NV_SRC_BACKGROUND);\n'
                       '  drawBackground(); SOURCEPERF_SCENE_END(NV_SRC_BACKGROUND); }\n')
        with_probe = commit()
        assert scope.diagnostic_change_safe(before_probe, with_probe, "src/app.c")
        app.write_text(app.read_text().replace("drawBackground()", "differentDraw()"))
        runtime_change = commit()
        assert not scope.diagnostic_change_safe(before_probe, runtime_change, "src/app.c")
        assert not scope.diagnostic_change_safe("1" * 40, runtime_change, "src/app.c")
        app.write_text("void draw(void) {\n  spainel_fundo(podeParar, cat_revisao(), desenharAtrasDoPainel, &agora); }\n")
        before_backdrop = commit()
        app.write_text('#include "source_backdrop.inc"\n' + app.read_text().replace("spainel_fundo(", "fontes_fundo("))
        with_backdrop = commit()
        assert scope.diagnostic_change_safe(before_backdrop, with_backdrop, "src/app.c")
        app.write_text(app.read_text().replace("cat_revisao()", "0"))
        altered_backdrop = commit()
        assert not scope.diagnostic_change_safe(before_backdrop, altered_backdrop, "src/app.c")
        main = path / 'src/main.c'
        main.write_text(swap); before_swap = commit()
        main.write_text(swap_probes); with_swap = commit()
        assert scope.diagnostic_change_safe(before_swap, with_swap, 'src/main.c')
        main.write_text(swap_probes.replace('SDL_GL_SwapWindow(win)', 'SDL_GL_SwapWindow(other)'))
        changed_swap = commit()
        assert not scope.diagnostic_change_safe(before_swap, changed_swap, 'src/main.c')
    finally:
        os.chdir(previous)
with tempfile.TemporaryDirectory(prefix="nuvio-test-scope-") as directory:
    output = Path(directory) / "result"
    env = {**os.environ, "NUVIO_TEST_HEAD": "1" * 40, "NUVIO_TEST_BASE": "2" * 40,
           "GITHUB_OUTPUT": str(output)}
    subprocess.run(["python3", str(ROOT / ".github/build/test_scope.py")], env=env, check=True, cwd=directory)
    assert output.read_text() == "scope=full\nfull=true\n"
print("PASS: source-only CI selection, shared dependency fallback, explicit full run and unavailable-history fallback")

with tempfile.TemporaryDirectory(prefix='nuvio-skip-scope-git-') as directory:
    path=Path(directory)
    subprocess.run(['git','init','-q',directory],check=True)
    def record():
        subprocess.run(['git','add','.'],cwd=path,check=True)
        subprocess.run(['git','-c','user.name=CI fixture','-c','user.email=fixture@example.invalid',
                        'commit','-qm','fixture'],cwd=path,check=True)
        return subprocess.check_output(['git','rev-parse','HEAD'],cwd=path,text=True).strip()
    def hashes():
        return {p:subprocess.check_output(['git','hash-object',p],cwd=path,text=True).strip()
                for p in scope.SKIP_BUTTON_APPLICATION}
    for p in scope.SKIP_BUTTON_APPLICATION:
        target=path/p;target.parent.mkdir(parents=True,exist_ok=True);target.write_text('before '+p+'\n')
    before_hashes=hashes();base=record()
    for p in scope.SKIP_BUTTON_APPLICATION:
        (path/p).write_text('after '+p+'\n')
    import json
    target=path/'.github/build/skip_buttons_scope.json';target.parent.mkdir(parents=True,exist_ok=True)
    target.write_text(json.dumps({'before':before_hashes,'after':hashes()}));head=record()
    previous=Path.cwd()
    try:
        os.chdir(path)
        paths=scope.changed(base,head)
        assert scope.skip_buttons_change_safe(base,head,paths)
        assert not scope.skip_buttons_change_safe(base,head,paths+['src/rede.c'])
        assert not scope.skip_buttons_change_safe(base,head,[])
        assert not scope.skip_buttons_change_safe('1'*40,head,paths)
        (path/'src/player.c').write_text('unrelated playback change\n');bad=record()
        assert not scope.skip_buttons_change_safe(base,bad,scope.changed(base,bad))
        (path/'src/player.c').write_text('after src/player.c\n');record()
        pin=json.loads(target.read_text());pin['after'].pop('src/intro.c');target.write_text(json.dumps(pin));bad=record()
        assert not scope.skip_buttons_change_safe(base,bad,paths)
    finally:
        os.chdir(previous)
print('PASS: focused skip-button tests require exact verified application blobs; unknown paths, missing history and later shared-code edits retain full regressions')
