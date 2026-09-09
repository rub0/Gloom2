# Audio original — hito 68

El inventario reproducible se genera con `tools/legacy/audit_audio.py LEGACY_ROOT`.
Escanea referencias de código y mapas, elimina comentarios C++, identifica ausentes
y duplicados por SHA-256 y decodifica todos los WAV/OGG/MP3 para verificarlos.
La selección explícita importa los sonidos del gameplay restaurado, las seis
habilidades originales y el tema de `Application/MenuState.cpp`; `troll` queda fuera.
`--import-assets` conserva los originales y cocina PCM flotante GAU1;
`--check` comprueba inventario, originales y cocción byte a byte. No hay rutas
absolutas en el inventario ni dependencia de FMOD o Python en el ejecutable.

Los sonidos breves y los cuatro recursos largos se precargan una vez y comparten
PCM entre voces. El formato cocinado mantiene canales, frecuencia y muestras del
decodificador; el mezclador interpola a 48 kHz estéreo. La carga valida cabecera,
tamaño exacto, límites de canales/frecuencia/duración y todas las muestras finitas.
Los originales se resuelven en `game:/audio`, el contenido cocinado en
`cache:/audio`. Volumen maestro, música y efectos son independientes.

Listener: cámara local, posición, frente, arriba y velocidad. Audio propio estéreo;
impactos, objetos y ambiente espacializados. Paneo de potencia constante y caída
inversa suave (distancia mínima 7,5 m, conversión de los 50 Legacy); distancia
máxima 150 m con fundido suave. Velocidad se conserva sin Doppler para evitar
oscilaciones causadas por reconciliación.

Pasos: cada 365 ms de movimiento real con apoyo, sin pasos en aire o contra pared.
Aterrizaje a partir de velocidad física previa al contacto: `-0,7 * 0,15 / 0,016`
= -6,5625 m/s; gruñido por debajo de -18,75 m/s. Nunca depende de animaciones.

Política: música continua entre menú y partida. Pausa silencia ambiente y efectos
y atenúa música al 50 %. Salir/reconectar detiene voces de escena y restablece la
línea base de eventos. Backend SDL3 encapsulado, con fallback nulo ante ausencia
de dispositivo. Dedicado y headless no inicializan audio.

Los eventos semánticos autoritativos llevan secuencia y posición, nunca muestras.
El primer snapshot establece la línea base: no reproduce eventos históricos.
Las confirmaciones repetidas se descartan. No se predicen one-shots locales:
se presentan desde la misma fuente autoritativa que los remotos, una única vez.
Los bucles vigentes se reconstruyen desde estado actual. Los campos introducidos
en el hito 68 requirieron protocolo 19. El hito 69 usa protocolo 20: replica la
clase explosiva del impacto e incorpora el evento 3D del jumper. Cada bola de
IronHellGoat emite un solo clip de impacto; se eliminó la superposición simultánea
de `fireball_hit` y `explotion`.

El hito 70 usa protocolo 21 e incorpora ocho archivos para Bite, Berserker,
Diamond Skin, Life Dome, Invisibility y Flash. Los one-shots se deduplican con el
diario autoritativo; el loop de Shadow se reconstruye desde el estado vigente.
El inventario resultante contiene 47 recursos importados de 89 auditados; el hito 75
añade `feedback/bell.mp3` para el aviso local de racha.

La revisión auditiva humana y el smoke de dispositivo se registran separadamente
de las pruebas del mezclador y de eventos; una prueba numérica no certifica timbre.
