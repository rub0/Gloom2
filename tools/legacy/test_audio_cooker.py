"""Run with the Legacy root argument; no audio hardware is used."""
import sys
import tempfile
from pathlib import Path
from audit_audio import decode

root = Path(sys.argv[1]) / "Exes/media/audio"
for name in ("character/jump.wav", "ambient/loop_deep.ogg", "feedback/bell.mp3"):
    meta, cooked = decode(root / name)
    assert meta["frames"] > 0 and cooked[:4] == b"GAU1"
    assert len(cooked) == 16 + meta["frames"] * meta["channels"] * 4
with tempfile.TemporaryDirectory(prefix="gloom-audio-") as temporary:
    for extension in ("wav", "ogg", "mp3"):
        path = Path(temporary) / ("broken." + extension)
        path.write_bytes(b"RIFF\x00\x00bad audio")
        try:
            decode(path)
        except ValueError as error:
            assert "audio decode failed" in str(error) and path.name in str(error)
        else:
            raise AssertionError(f"corrupt {extension} accepted")
print("WAV/OGG/MP3 decode and named corruption diagnostics passed")
