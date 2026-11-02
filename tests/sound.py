#!/usr/bin/env python3
"""Check chip and speaker selection through the compiled Sound page.

Copyright (c) 2015-2026, BogDan Vatra. GPL-3.0-or-later.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile


def main():
    repo = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=repo / "build/bin/spectacol")
    parser.add_argument("--output", type=Path, default=repo / "build/sound")
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="session-", dir=output) as temporary:
        session = Path(temporary)
        config = session / "config/Licentia"
        config.mkdir(parents=True)
        ini = config / "Spectacol.conf"
        ini.write_text("[General]\nfirst_start=false\nautoSaveOnExit=false\ndataPath="
                       + str(session / "data") + "\n")
        env = os.environ | {"QT_QPA_PLATFORM": "offscreen", "XDG_CONFIG_HOME": str(config.parent),
                            "XDG_DATA_HOME": str(session / "data")}
        open_sound = "click:320:24,Down,Down,Down,Down,Return"

        def run(name, keys):
            command = [str(args.binary.resolve()), "--machine", "128k", "--quit-after", "6",
                       "--window-size", "1280x800", "--ui-keys", keys,
                       "--ui-screenshot", str(output / f"{name}.png")]
            with (output / f"{name}.log").open("w") as log:
                subprocess.run(command, env=env, stdout=log, stderr=subprocess.STDOUT,
                               check=True, timeout=25)
            log = (output / f"{name}.log").read_text()
            assert not any(error in log for error in ("ReferenceError", "TypeError", "Unknown key")), log

        # Down/Right are the same navigation keys delivered by the gamepad UI.
        run("ym", open_sound + ",Down,Down,Down,Right")
        assert "ayModel=YM2149" in ini.read_text(), ini.read_text()
        # Startup restores YM; one Left press must return it to AY.
        run("ay", open_sound + ",Down,Down,Down,Left")
        assert "ayModel=AY8912" in ini.read_text(), ini.read_text()
        # Older/bad settings fall back to AY, allowing selection normally.
        ini.write_text(ini.read_text().replace("ayModel=AY8912", "ayModel=unknown"))
        run("recovered", open_sound + ",Down,Down,Down,Right")
        assert "ayModel=YM2149" in ini.read_text(), ini.read_text()
        speaker_keys = open_sound + ",Down,Down,Down,Down"
        run("speaker-beeper", speaker_keys + ",Right")
        assert "speakerType=Beeper" in ini.read_text(), ini.read_text()
        run("speaker-unfiltered", speaker_keys + ",Right")
        assert "speakerType=Unfiltered" in ini.read_text(), ini.read_text()
        run("speaker-restored", speaker_keys + ",Left")
        assert "speakerType=Beeper" in ini.read_text(), ini.read_text()
        run("speaker-tv", speaker_keys + ",Left")
        assert "speakerType=TV speaker" in ini.read_text(), ini.read_text()
        ini.write_text(ini.read_text().replace("speakerType=TV speaker", "speakerType=unknown"))
        run("speaker-recovered", speaker_keys + ",Right")
        assert "speakerType=Beeper" in ini.read_text(), ini.read_text()
    print("Chip/speaker navigation, persistence and invalid-setting recovery passed")


if __name__ == "__main__":
    main()
