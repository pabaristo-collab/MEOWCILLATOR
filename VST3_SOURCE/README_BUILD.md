# MEOWCILLATOR 0.9.6 — Windows VST3 build / REAPER

**Esto es el proyecto de compilación, NO un plugin VST3 precompilado.** Para Windows x64 se debe ejecutar la compilación usando JUCE 8.0.15 y Visual Studio 2022 (o GitHub Actions). Este paquete NO ha sido compilado con JUCE ni cargado en REAPER; no se garantiza que compile sin ajustes.

## CONTENIDO
- `VST3_SOURCE/`: código C++ JUCE del VST3 (37 presets, 11 patrones, 6 sonidos felinos sintetizados, BLACK CAT por defecto).
- `VST3_SOURCE/BUILD_WINDOWS.bat`: compilación local, descarga JUCE 8.0.15.
- `VST3_SOURCE/.github/workflows/build-vst3-windows.yml`: flujo opcional para compilar en Windows mediante GitHub Actions. Debe subirse el contenido de `VST3_SOURCE` a la raíz de un repositorio nuevo.
- `REAPER_JSFX_CORREGIDO/`: parche temporal que soluciona el error `'tanh' undefined` visto en REAPER.
- `DEMO_WEB_095.html`: última demo web; no es VST3.

## Compilación local (Windows)
1. Instalar Visual Studio 2022 Community con el módulo **Desktop development with C++** (incluye MSVC y Windows SDK).
2. Instalar **CMake** y **Git for Windows**; comprobar que están en PATH.
3. Extraer el ZIP en una ruta corta, por ejemplo `C:\MEOWCILLATOR\`.
4. Abrir `VST3_SOURCE/BUILD_WINDOWS.bat` con doble clic. Requiere conexión para descargar JUCE.
5. Si compila, copiar completa la carpeta `VST3_SOURCE/build/Meowcillator_artefacts/Release/VST3/MEOWCILLATOR.vst3` a `C:\Program Files\Common Files\VST3\`. Windows puede pedir permiso de administrador.
6. REAPER > Options > Preferences > Plug-ins > VST > Re-scan; buscar `VST3i: MEOWCILLATOR`.

## Alternativa sin Visual Studio local: GitHub Actions
1. Crear un repositorio GitHub privado (si tu cuenta permite Actions) o público.
2. Subir el **contenido** de la carpeta `VST3_SOURCE` como raíz del repositorio (incluye `.github/workflows`).
3. Abrir la pestaña **Actions**, elegir `Compilar MEOWCILLATOR VST3 - Windows x64` > Run workflow.
4. Si el build termina con éxito, descargar el artifact `MEOWCILLATOR-096-Windows-VST3`.
5. Extraerlo asegurando estructura VST3 válida, y copiar la carpeta `MEOWCILLATOR.vst3` a la ruta anterior. Si el artifact contiene solo `Contents/`, renombrar la carpeta extraída a `MEOWCILLATOR.vst3` (no un archivo solo).

## Reparar el JSFX que aparece en la captura
1. REAPER > Options > Show REAPER resource path in explorer.
2. Entrar a `Effects` y reemplazar el archivo `MEOWCILLATOR_REAPER_TEST.jsfx` por la edición ubicada en `REAPER_JSFX_CORREGIDO`.
3. En la ventana FX, pulsar Edit > Recompile (o quitar y volver a insertar el JSFX).
4. Ya no debería aparecer `@sample ... 'tanh' undefined`. Esto corrige el error específico, pero no equivale a pruebas de audio en REAPER.

## Cambios desde 0.9.5
- JUCE: tema `ORANGE CAT` realmente naranja, y los colores BLUSH/ICE/ACID corregidos.
- JUCE: dibujado de fondo escalado correctamente.
- JSFX: reemplazo de tanh() ausente por `soft_tanh()` definido en EEL2; título responde al tema.
- JUCE 8.0.15 fijado para reproducibilidad; scripts de build Windows y GitHub Actions.

No se incluye ningún plugin `.vst3` compilado ni un instalador. Evita renombrar `.jsfx` o `.dll` a `.vst3`; no los convierte en un plugin válido.
