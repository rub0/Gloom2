# Cinco checks nativos de rutas

Datos de diagnóstico del hito 117. Capturas/builds/blobs permanecen en caché.

```json
[
  {
    "case": "Unicode roots, filename and percent-decoded image URI",
    "exit": 0,
    "stdout": "Cooked scene 1130519048210987007 with 3 external dependencies.\n",
    "stderr": ""
  },
  {
    "case": "traversal",
    "exit": 2,
    "stdout": "",
    "stderr": "Source and output must be valid game:/ and cache:/ virtual paths.\n"
  },
  {
    "case": "missing mount root",
    "exit": 1,
    "stdout": "",
    "stderr": "Virtual filesystem mount name or directory is invalid\n"
  },
  {
    "case": "junction escape",
    "exit": 1,
    "stdout": "",
    "stderr": "Asset cooking failed: Resolved asset path escapes its mount\n"
  },
  {
    "case": "ill-formed UTF-16",
    "exit": 2,
    "stderr": "Asset cooker argument is not valid Unicode.\n"
  }
]
```
