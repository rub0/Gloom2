# Probe de carga del cierre

Datos de diagnóstico del hito 117. Capturas/builds/blobs permanecen en caché.

```json
{
  "probe_sha256": "c789e9b028490c5d98f214b951af46b199cdda31f30e101929dd7e96b9bf4115",
  "libraries": {
    "gloom_engine.lib": "bf9dd5a2d136f4f63c4e26fa55180b544e7be02f0bd0c56b2c39fb0a6d670da2",
    "gloom_asset_pipeline.lib": "cb3ce67eff87a7e7c97d690073314dfc22937b3c77bdb66c4dfa588fb617a1ce"
  },
  "runs": [
    {
      "mode": "normal",
      "case": "small",
      "trial": 0,
      "output": "stage=0 ns=182598.000\nstage=1 ns=2669.000\nchecksum=12396970790278684612\n"
    },
    {
      "mode": "normal",
      "case": "small",
      "trial": 1,
      "output": "stage=0 ns=168199.000\nstage=1 ns=2723.000\nchecksum=12396970790278684612\n"
    },
    {
      "mode": "normal",
      "case": "small",
      "trial": 2,
      "output": "stage=0 ns=173524.000\nstage=1 ns=2706.000\nchecksum=12396970790278684612\n"
    },
    {
      "mode": "normal",
      "case": "small",
      "trial": 3,
      "output": "stage=0 ns=170179.000\nstage=1 ns=2716.000\nchecksum=12396970790278684612\n"
    },
    {
      "mode": "normal",
      "case": "small",
      "trial": 4,
      "output": "stage=0 ns=166712.000\nstage=1 ns=2696.000\nchecksum=12396970790278684612\n"
    },
    {
      "mode": "normal",
      "case": "factory",
      "trial": 0,
      "output": "stage=0 ns=21740525.000\nstage=1 ns=11237140.000\nchecksum=13572051394366891560\n"
    },
    {
      "mode": "normal",
      "case": "factory",
      "trial": 1,
      "output": "stage=0 ns=22033680.000\nstage=1 ns=11037100.000\nchecksum=13572051394366891560\n"
    },
    {
      "mode": "normal",
      "case": "factory",
      "trial": 2,
      "output": "stage=0 ns=22345980.000\nstage=1 ns=11543340.000\nchecksum=13572051394366891560\n"
    },
    {
      "mode": "normal",
      "case": "factory",
      "trial": 3,
      "output": "stage=0 ns=22468390.000\nstage=1 ns=11073235.000\nchecksum=13572051394366891560\n"
    },
    {
      "mode": "normal",
      "case": "factory",
      "trial": 4,
      "output": "stage=0 ns=21692720.000\nstage=1 ns=11816800.000\nchecksum=13572051394366891560\n"
    },
    {
      "mode": "normal",
      "case": "hound",
      "trial": 0,
      "output": "stage=0 ns=37098120.000\nstage=1 ns=16666120.000\nchecksum=15699014412979931593\n"
    },
    {
      "mode": "normal",
      "case": "hound",
      "trial": 1,
      "output": "stage=0 ns=34692620.000\nstage=1 ns=16683640.000\nchecksum=15699014412979931593\n"
    },
    {
      "mode": "normal",
      "case": "hound",
      "trial": 2,
      "output": "stage=0 ns=35487580.000\nstage=1 ns=16652720.000\nchecksum=15699014412979931593\n"
    },
    {
      "mode": "normal",
      "case": "hound",
      "trial": 3,
      "output": "stage=0 ns=35578020.000\nstage=1 ns=16565260.000\nchecksum=15699014412979931593\n"
    },
    {
      "mode": "normal",
      "case": "hound",
      "trial": 4,
      "output": "stage=0 ns=36444180.000\nstage=1 ns=16644120.000\nchecksum=15699014412979931593\n"
    },
    {
      "mode": "memory",
      "case": "small",
      "trial": 0,
      "output": "stage=0 calls=23 bytes=4368 peak=3456 live_delta=0\nstage=1 calls=1 bytes=976 peak=976 live_delta=0\nchecksum=8056069659597894041\n"
    },
    {
      "mode": "memory",
      "case": "factory",
      "trial": 0,
      "output": "stage=0 calls=212 bytes=12159507 peak=12158595 live_delta=0\nstage=1 calls=1 bytes=4045999 peak=4045999 live_delta=0\nchecksum=14513660625000508290\n"
    },
    {
      "mode": "memory",
      "case": "hound",
      "trial": 0,
      "output": "stage=0 calls=1447 bytes=18729066 peak=18724186 live_delta=0\nstage=1 calls=1 bytes=6227136 peak=6227136 live_delta=0\nchecksum=10518500512079806965\n"
    }
  ],
  "fixtures": {
    "small": "c0694a70ba3dc80d85d00c39e29f139e81a7d6b8189b641209d6058f2f096fcc",
    "factory": "297d201df6a645ce8cdb95e461a5a17dc0f0e01d137351f57091f7ae01a131ef",
    "hound": "b2829a1a54ff0b2f521b1c243549ef63a9cddade2cac147f0c43df8cbc8e5f0e"
  },
  "executables": {
    "normal": "b22ccf509fa62fb0115a3b7bfa2af90e11f55f83f4fcc2db66ad9ad88e986709",
    "memory": "01fa80d1d1b71012b89b37619e98969b3e5ca840b5c42adeff7660967317175d"
  }
}
```
