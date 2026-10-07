# Comprobación de la herramienta final

Datos de diagnóstico del hito 117. Capturas/builds/blobs permanecen en caché.

```json
{
  "probe_sha256": "0a2e0e1212816826039096a0efcada4c1c479a8e2959e411e37908f67cb43bfe",
  "fixtures": {
    "small": "c0694a70ba3dc80d85d00c39e29f139e81a7d6b8189b641209d6058f2f096fcc",
    "factory": "297d201df6a645ce8cdb95e461a5a17dc0f0e01d137351f57091f7ae01a131ef",
    "hound": "b2829a1a54ff0b2f521b1c243549ef63a9cddade2cac147f0c43df8cbc8e5f0e"
  },
  "executables": {
    "normal": "56d24ed93f014e482027dac7b28ae7735842394c22db46014c0a17f0d0c93b69",
    "memory": "9c55d1c8abb2496b59a44f7ed148aeb7e19c77d7ba855692911e56964dbfd94f"
  },
  "libraries": {
    "gloom_engine.lib": "bf9dd5a2d136f4f63c4e26fa55180b544e7be02f0bd0c56b2c39fb0a6d670da2",
    "gloom_asset_pipeline.lib": "cb3ce67eff87a7e7c97d690073314dfc22937b3c77bdb66c4dfa588fb617a1ce"
  },
  "runs": [
    {
      "mode": "normal",
      "case": "small",
      "trial": 0,
      "output": "stage=0 ns=169715.000\nstage=1 ns=2732.000\nchecksum=12396970790278684612\n"
    },
    {
      "mode": "normal",
      "case": "small",
      "trial": 1,
      "output": "stage=0 ns=169793.000\nstage=1 ns=2711.000\nchecksum=12396970790278684612\n"
    },
    {
      "mode": "normal",
      "case": "small",
      "trial": 2,
      "output": "stage=0 ns=168482.000\nstage=1 ns=2796.000\nchecksum=12396970790278684612\n"
    },
    {
      "mode": "normal",
      "case": "small",
      "trial": 3,
      "output": "stage=0 ns=184938.000\nstage=1 ns=2686.000\nchecksum=12396970790278684612\n"
    },
    {
      "mode": "normal",
      "case": "small",
      "trial": 4,
      "output": "stage=0 ns=171795.000\nstage=1 ns=2718.000\nchecksum=12396970790278684612\n"
    },
    {
      "mode": "normal",
      "case": "factory",
      "trial": 0,
      "output": "stage=0 ns=22107435.000\nstage=1 ns=11526435.000\nchecksum=13572051394366891560\n"
    },
    {
      "mode": "normal",
      "case": "factory",
      "trial": 1,
      "output": "stage=0 ns=22903655.000\nstage=1 ns=11434725.000\nchecksum=13572051394366891560\n"
    },
    {
      "mode": "normal",
      "case": "factory",
      "trial": 2,
      "output": "stage=0 ns=22191400.000\nstage=1 ns=11045330.000\nchecksum=13572051394366891560\n"
    },
    {
      "mode": "normal",
      "case": "factory",
      "trial": 3,
      "output": "stage=0 ns=22379520.000\nstage=1 ns=11184080.000\nchecksum=13572051394366891560\n"
    },
    {
      "mode": "normal",
      "case": "factory",
      "trial": 4,
      "output": "stage=0 ns=21968260.000\nstage=1 ns=11153030.000\nchecksum=13572051394366891560\n"
    },
    {
      "mode": "normal",
      "case": "hound",
      "trial": 0,
      "output": "stage=0 ns=35064520.000\nstage=1 ns=16611240.000\nchecksum=15699014412979931593\n"
    },
    {
      "mode": "normal",
      "case": "hound",
      "trial": 1,
      "output": "stage=0 ns=34807740.000\nstage=1 ns=16600120.000\nchecksum=15699014412979931593\n"
    },
    {
      "mode": "normal",
      "case": "hound",
      "trial": 2,
      "output": "stage=0 ns=34617780.000\nstage=1 ns=16754820.000\nchecksum=15699014412979931593\n"
    },
    {
      "mode": "normal",
      "case": "hound",
      "trial": 3,
      "output": "stage=0 ns=34926680.000\nstage=1 ns=16661480.000\nchecksum=15699014412979931593\n"
    },
    {
      "mode": "normal",
      "case": "hound",
      "trial": 4,
      "output": "stage=0 ns=34888680.000\nstage=1 ns=16622060.000\nchecksum=15699014412979931593\n"
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
  ]
}
```
