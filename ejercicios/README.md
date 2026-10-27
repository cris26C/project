# Práctica 3 — Sesión 1

Cada archivo es el `main/main.c` completo de un ejercicio. Para probar uno, cópialo encima de `main/main.c` y compila.

| Archivo | Ejercicio | Comunicación entre tareas | `PRIV_REQUIRES` en `main/CMakeLists.txt` |
|---|---|---|---|
| `ejercicio1.c` | 1. Tarea muestreadora | Variable global | `shtc3` |
| `ejercicio2.c` | 2. Colas | `xQueueSend` / `xQueueReceive` | `shtc3` |
| `ejercicio3.c` | 3. Eventos | `esp_event_post_to` + handler en un loop propio | `shtc3 esp_event` |

## Cuestiones

**Ejercicio 1**
- Prioridad de `app_main`: 1. Se consulta con `uxTaskPriorityGet(NULL)`.
- Sincronización: no hay. `app_main` imprime la global periódicamente sin saber si el dato es nuevo; puede repetir o perder muestras, o leer una a medio escribir.
- Dos argumentos (período + dirección destino): se agrupan en un struct `static` y se pasa su dirección como `void *`.

**Ejercicio 2**
- Los datos de una cola se pasan **por copia** (`xQueueCreate` recibe el tamaño de cada elemento).

**Ejercicio 3**
- Tras registrar el handler, `app_main` no tiene que hacer nada más: imprime la tarea del event loop.
- Puede finalizar, siempre que los argumentos de la tarea sean `static` (no estén en la pila de `app_main`).
