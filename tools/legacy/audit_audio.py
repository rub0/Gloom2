"""Reproducible Legacy audio inventory and strict offline WAV/OGG/MP3 cooker.

Usage: audit_audio.py LEGACY_ROOT [--import-assets] [--check]
Runtime consumes validated PCM, never requires Python, FFmpeg or FMOD.
"""
import argparse
import hashlib
import json
import re
import shutil
import struct
from pathlib import Path

import av
import numpy as np

ROOT = Path(__file__).resolve().parents[2]
SELECTED = """footsteps/step1.wav character/jump.wav character/sideJump.wav
land/land.wav damage/landingMaleGrunt.wav damage/pain.wav damage/splatdeath_03.wav
gameplay/spawn.wav weapons/noammo.wav weapons/change.wav weapons/soulReaper/miss.wav
weapons/soulReaper/wallHit.wav weapons/soulReaper/wallHit2.wav weapons/soulReaper/goreHit.wav
weapons/shotgun/shotgun.wav weapons/sniper/sniper.wav weapons/minigun/shoot.wav
weapons/ironHellGoat/shootFireBall2.wav weapons/hit/elec_ric.wav weapons/hit/ric2.wav
weapons/hit/ric3.wav weapons/hit/fireball_hit.wav weapons/explotion.wav
items/armor.wav items/holdable.wav items/healthPack.wav items/healthVial.wav
weapons/ammoPickup.wav weapons/shotgun/shotgunPickup.wav weapons/sniper/sniperPickup.wav
weapons/minigun/minigunPickup.wav weapons/ironHellGoat/ironHellGoatPickup.wav
music/themeGloom.wav ambient/lava_quiet.wav ambient/bassy_fan.wav ambient/deep_atmosphere.wav
weapons/ironHellGoat/ignite_pitch.wav gameplay/plasma.wav""".split()


def decode(path):
    try:
        with av.open(str(path), metadata_errors="replace") as container:
            if len(container.streams.audio) != 1:
                raise ValueError("expected exactly one audio stream")
            stream = container.streams.audio[0]
            rate = stream.codec_context.sample_rate
            channels = stream.codec_context.channels
            if not 8000 <= rate <= 192000 or channels not in (1, 2):
                raise ValueError(f"unsupported rate/channels: {rate}/{channels}")
            resampler = av.AudioResampler(format="fltp", layout="mono" if channels == 1 else "stereo", rate=rate)
            blocks = []
            for frame in container.decode(stream):
                blocks.extend(f.to_ndarray().T for f in resampler.resample(frame))
            blocks.extend(f.to_ndarray().T for f in resampler.resample(None))
            pcm = np.concatenate(blocks)
            if not np.isfinite(pcm).all() or not 0 < len(pcm) / rate <= 1800:
                raise ValueError("empty, nonfinite or excessive duration")
            meta = dict(format=container.format.name, channels=channels, sample_rate=rate,
                        frames=len(pcm), duration_seconds=round(len(pcm) / rate, 6),
                        peak=round(float(np.max(np.abs(pcm))), 6))
            # Lossless float representation of decoder output; no creative processing.
            return meta, struct.pack("<4sIII", b"GAU1", rate, channels, len(pcm)) + pcm.astype("<f4").tobytes()
    except Exception as error:
        raise ValueError(f"{path.name}: audio decode failed: {error}") from error


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("legacy", type=Path)
    parser.add_argument("--import-assets", action="store_true")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    media = args.legacy / "Exes/media/audio"
    files = sorted(p for p in media.rglob("*") if p.is_file())
    refs = {}
    # Scan text only. Strip C/C++ comments so disabled experiments are not live refs.
    sources = sorted((args.legacy / "Src").rglob("*.cpp")) + sorted((args.legacy / "Exes/media/maps").glob("*.txt"))
    for source in sources:
        raw = source.read_text(encoding="latin1")
        raw = re.sub(r'/\*.*?\*/|//[^\n]*', lambda m: "\n" * m[0].count("\n"), raw, flags=re.S)
        for number, line in enumerate(raw.splitlines(), 1):
            for name in re.findall(r'"([^"\n]+\.(?:wav|ogg|mp3))"', line, re.I):
                refs.setdefault(name.lower(), []).append(f"{source.relative_to(args.legacy).as_posix()}:{number}")
    selected = {name.lower() for name in SELECTED}
    records, hashes = [], {}
    for path in files:
        name = path.relative_to(media).as_posix()
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        hashes.setdefault(digest, []).append(name)
        metadata, cooked = decode(path)
        included = name.lower() in selected
        record = dict(path=name, sha256=digest, references=refs.get(name.lower(), []),
                      imported=included, status="excluded_troll" if name.lower().startswith("troll/") else
                      "referenced" if name.lower() in refs else "orphan", **metadata)
        if included and not record["references"]:
            record["selection_reason"] = {"weapons/explotion.wav": "Original explosion sample for authoritative fireball contacts",
                "weapons/ironhellgoat/ignite_pitch.wav": "Original ignition sample for current weapon charge/guidance loops"}[name.lower()]
        records.append(record)
        if included:
            target = ROOT / "assets/audio/original" / name
            cache = ROOT / "assets/audio/cooked" / (name.lower() + ".gau")
            if args.import_assets:
                target.parent.mkdir(parents=True, exist_ok=True)
                cache.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(path, target)
                cache.write_bytes(cooked)
            if args.check and (not target.exists() or target.read_bytes() != path.read_bytes() or not cache.exists() or cache.read_bytes() != cooked):
                raise ValueError(f"stale imported/cooked audio: {name}")
    available = {p.relative_to(media).as_posix().lower() for p in files}
    if selected - available:
        raise ValueError(f"missing selected assets: {sorted(selected - available)}")
    inventory = dict(version=1, files=records,
                     missing_references={k: v for k, v in sorted(refs.items()) if k not in available},
                     duplicates=[v for _, v in sorted(hashes.items()) if len(v) > 1])
    destination = ROOT / "assets/audio/inventory.json"
    encoded = json.dumps(inventory, indent=2, ensure_ascii=False) + "\n"
    if args.check:
        if destination.read_text(encoding="utf8") != encoded:
            raise ValueError("audio inventory is stale")
    else:
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_text(encoded, encoding="utf8")
    print(f"Audio audit: {len(records)} files, {len(selected)} imported, {len(inventory['missing_references'])} missing references, {len(inventory['duplicates'])} duplicate groups")


if __name__ == "__main__":
    main()
