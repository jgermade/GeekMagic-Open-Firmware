## Plan: Reinicio Firmware ESP8266 con OTA y Captive Portal

Reiniciar el firmware en la raiz del proyecto usando el mismo hardware (esp12e + display), preservando compatibilidad OTA legacy con ESP8266HTTPUpdateServer y la conectividad WiFi, e incorporando portal cautivo cuando STA no conecte. Se reutilizan patrones estables de legacy, pero con codigo nuevo en src/ e include/, dejando legacy intacto.

## Estado Actual (26-05-2026)

- Fase base implementada en raiz.
- Build verificado: SUCCESS para env esp12e.
- OTA legacy activo en /legacyupdate.
- OTA API moderna activa (/api/v1/ota/fw, /api/v1/ota/status, /api/v1/ota/cancel).
- WiFi STA/AP con fallback a AP portal.
- Captive portal con DNSServer y rutas tipicas de deteccion.
- Display basico integrado para estado de red y progreso OTA.

## Steps

1. Fase 1 - Baseline del proyecto nuevo (bloqueante)
2. Crear estructura minima en raiz para firmware nuevo: src/main.cpp, modulos en src/wireless, src/web, src/config, src/ota, y headers en include/ homologos. Reusar platformio.ini existente (esp12e, LittleFS, 4MB, eagle.flash.4m2m.ld).
3. Definir contrato de configuracion inicial (SSID/PASS STA, token API, parametros AP portal, flags de modo). Mantener persistencia en LittleFS/SecureStorage con migracion limpia desde estado vacio (sin depender de data legacy).
4. Fase 2 - WiFi STA/AP con estado explicito (bloquea Fase 3)
5. Implementar WiFiManager nuevo con maquina de estados: BOOT, STA_CONNECTING, STA_CONNECTED, AP_PORTAL. Si STA falla, entrar en AP_PORTAL permanente (decision validada) y mantenerlo hasta configuracion exitosa.
6. Implementar monitor de enlace WiFi en loop para detectar caida de STA y volver a AP_PORTAL sin reboot forzado. Publicar metodo update() no bloqueante para uso continuo.
7. Fase 3 - Captive portal completo (depende de 5 y 6)
8. Integrar DNSServer para captura de DNS en modo AP (wildcard a IP del AP) y rutas de deteccion de captive portal (Android/Apple/Windows) devolviendo redireccion/control HTTP apropiado hacia la landing del portal.
9. Implementar landing portal en AP con formulario WiFi y endpoint de conexion rapida. Al exito de conexion STA: guardar credenciales, desactivar AP/DNS, cambiar estado a STA_CONNECTED y confirmar al cliente.
10. Mantener AP abierto temporal (sin password), solo en modo portal, y deshabilitarlo al recuperar STA.
11. Fase 4 - OTA dual compatible (parcialmente en paralelo con Fase 3 frontend)
12. Registrar compatibilidad legacy con ESP8266HTTPUpdateServer en ruta /legacyupdate usando el servidor HTTP crudo (raw) del wrapper Webserver.
13. Implementar endpoints OTA API nuevos (firmware y opcional FS), con estado/progreso/cancelacion y autenticacion por token para API moderna; mantener /legacyupdate por compatibilidad sin romper clientes existentes.
14. Anadir protecciones OTA: validacion de espacio disponible, manejo robusto de errores Update, y respuesta de estado consistente para UI remota.
15. Fase 5 - UI minima + display local (depende de 8, 9, 12, 13)
16. Servir una UI web minima para portal y estado (sin arrastrar todo el frontend legacy), priorizando configuracion WiFi y visibilidad de OTA.
17. Conservar feedback basico en pantalla local: estado de conexion (STA/AP), IP activa y progreso OTA en upload.
18. Fase 6 - Endurecimiento y limpieza (final)
19. Ajustar watchdog y tiempos de loop para evitar resets en cargas OTA largas y durante procesamiento DNS/HTTP.
20. Definir limites de alcance: sin GIF, sin reproductor multimedia y sin restaurar todos los endpoints legacy no esenciales en la primera entrega.
21. Documentar en readme.md la arquitectura nueva, endpoints soportados y matriz de compatibilidad (legacyupdate + API nueva).

## Relevant Files

- platformio.ini
- src/main.cpp
- include/wireless/WiFiManager.h
- src/wireless/WiFiManager.cpp
- include/ota/OtaManager.h
- src/ota/OtaManager.cpp
- include/config/ConfigManager.h
- src/config/ConfigManager.cpp
- include/display/DisplayManager.h
- src/display/DisplayManager.cpp
- include/web/Routes.h
- src/web/Routes.cpp
- legacy/src/main.cpp
- legacy/src/wireless/WiFiManager.cpp
- legacy/src/web/Api.cpp

## Verification

1. Compilar limpio con PlatformIO para env esp12e.
2. Prueba de arranque sin credenciales: AP_PORTAL y redireccion DNS funcional.
3. Prueba de provisioning: conectar STA, persistir credenciales, desactivar AP y confirmar IP STA.
4. Prueba de perdida de WiFi: simular desconexion de router y validar retorno automatico a AP_PORTAL.
5. Prueba OTA legacy: subir bin por /legacyupdate y validar reinicio exitoso.
6. Prueba OTA moderna: upload por endpoint API con consulta de estado/cancelacion.
7. Prueba display: mostrar estado AP/STA y progreso OTA visible localmente.
8. Verificar memoria libre y estabilidad de watchdog durante OTA y captive portal simultaneos.

## Decisions

- Incluye: firmware nuevo en raiz, OTA dual (legacy + API nueva), WiFi STA/AP, captive portal, display basico de estado.
- Excluye (fase inicial): GIF/media, restauracion completa de todos endpoints legacy, features no criticas de UI antigua.
- Compatibilidad: se conserva /legacyupdate para no romper herramientas o flujos existentes.
- Seguridad AP: AP abierto temporal solo para provisioning, desactivado al recuperar STA.

## Build/Setup Rapido En Otro Ordenador

1. Configurar entorno Python del workspace.
2. Instalar PlatformIO en el venv del proyecto.
3. Ejecutar:

```bash
.venv/bin/python -m platformio run -e esp12e
```

## Further Considerations

1. Seguridad de provisioning: Opcion A mantener AP abierto como acordado; Opcion B cerrar con PIN temporal si se detectan despliegues en entornos hostiles.
2. OTA API scope: Opcion A habilitar solo firmware en v1; Opcion B habilitar tambien FS desde el inicio si ya existe pipeline de LittleFS.
3. UI del portal: Opcion A minima embebida para rapidez; Opcion B reutilizar parcialmente assets legacy para reducir trabajo visual.
