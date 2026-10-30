# Architektur der technischen Grundlage

`src/app` verdrahtet den Lebenszyklus. QML kennt `AppSettings` und den Playback-Controller `MpvEngine` sowie `LibraryController`, keine SQL-Verbindung oder rohen mpv-Handles. Das reguläre ApplicationWindow nutzt Systemdekorationen und erzwingt weder X11 noch rahmenloses Fensterverhalten. Qt wählt das Plattform-Backend aus der Desktop-Sitzung.

| Schicht | Verantwortung / aktueller Umfang |
| --- | --- |
| qml | Feste obere Navigation, Bibliotheksraster, Suche, Assistent, Profilmenü, Einstellungen, Dark/Light-Farben |
| core | Theme-Auswahl, Persistenzfehler mit Rückmeldung an die Oberfläche |
| database | Verbindungslebenszyklus, gebundene SQL-Parameter, SQLite-Migrationen |
| player | libmpv-Lebenszyklus, asynchrone Wiedergabebefehle, Eventverarbeitung und getrennte Videoausgabe auf einem Worker-Thread |
| scanner | Rekursive Discovery, Dateinamenparser und ffprobe auf eigenem QThread; Pause/Fortsetzen/Abbruch |
| library | GUI-Controller, inkrementelle Listenmodelle, Sortier-/Filter-Proxies, Quellenbeobachtung |
| metadata | Austauschbarer Providervertrag und expliziter Einwilligungszustand; kein Netzwerkclient |
| übrige Module | Reservierte Modulverzeichnisse mit dokumentierter Verantwortung |

Der Scanner besitzt seine eigene DB-Verbindung auf dem Worker-Thread. Fortschritt und Snapshots erreichen die GUI über queued signals. Der Metadatenprovider bleibt ein Vertrag ohne Netzwerkimplementierung. Details: [LIBRARY.md](LIBRARY.md).

## Datenbank / Migrationen 001–002

`profiles(id, name)` enthält genau ein initiales Standardprofil. `app_settings(key, value)` enthält `active_profile=1` und `theme=midnight`. Migration 2 ergänzt `media_sources`, `media_titles`, `media_files`, `scan_errors` und `profile_preferences`. WAL wird nach erfolgreicher Migration eingeschaltet. Dateieinträge werden einzeln atomar gespeichert; Discovery/Medienanalyse halten keine lange Schreibtransaktion offen.

`PRAGMA user_version` steuert lückenlose Migrationen ab Version 1. Jede Version läuft atomar in einer Transaktion; bei Fehlern erfolgt Rollback. Bereits durchgeführte Migrationen werden übersprungen. Höhere Schemaversionen werden vor Änderungen abgelehnt. `foreign_keys=ON`, ein Busy-Timeout und `quick_check` sind aktiv. Es gibt keine automatische Datenbank-Neuanlage als Fehlerbehandlung und keinen automatischen Downgrade.

Die initiale Migration ist klein und läuft beim Start. Umfangreiche spätere Migrationen müssen vor ihrer Umsetzung auf Startzeit, Sicherung und Thread-Nutzung geprüft werden.

## Oberfläche und Umfang

Midnight Blue mit Cyan ist Standard, Light Mode ist auswählbar und persistent. Beide Designs verwenden dieselben semantischen Farbtokens einschließlich der Qt-Control-Palette. Tastaturfokus wird sichtbar dargestellt. Häufige Navigation hat keine verzögernde Animation.

Leere Startseitenreihen werden ausgeblendet; ohne Bibliothek zeigt die Startseite einen kompakten Leerzustand. Es gibt keinen Hero-Banner, keine Demofilme und keine vorgetäuschte Wiedergabe. Lokale Titel-Suche öffnet Film-/Seriendetails. Weitere Einstellungsbereiche sind ausdrücklich als späterer Entwicklungsschritt gekennzeichnet. Der sechsstufige Assistent speichert eine ausdrückliche Online-Entscheidung; ohne Provider erfolgen weiterhin keine Online-Abfragen. Sprache und Avatarwahl sind noch nicht vollständig umgesetzt.

## Grundlegende Wiedergabe

`MpvEngine` ist der QML-Controller für lokale Dateien und besitzt den mpv-Client über eine geteilte Session. Befehle und Property-Änderungen werden asynchron an mpv geschickt. Ein 30-ms-Timer leert den Ereignispuffer ohne blockierendes Warten. Dateipfade werden als absolute native Pfade übergeben; URLs für die unmittelbare Dateiauswahl müssen lokal sein. Benutzer-MPV-Konfiguration und automatisch geladene externe Skripte sind deaktiviert.

`VideoWorker` besitzt den libmpv-Software-Renderkontext auf einem eigenen QThread. Resize und Frame-Requests werden in diesen Thread eingereiht. Ein Mutex-geschützter Einzelbildpuffer hält nur das neueste Bild; höchstens eine GUI-Benachrichtigung wartet gleichzeitig. `VideoItem` zeichnet das unveränderliche Bild mit QQuickPaintedItem. Qt übernimmt die Darstellung unabhängig von OpenGL/Vulkan/Software-Backend; eigener OpenGL-Code ist in diesem Stand nicht enthalten.

Beim Abbau wird der Renderkontext auf seinem Worker freigegeben, bevor der Thread endet. Die geteilte Session bleibt bis danach erhalten; erst anschließend darf mpv beendet werden. Schließen und Navigation stoppen die Wiedergabe. Die numerische C-Locale wird nach der Qt-Anwendungsinitialisierung für libmpv hergestellt; Qt-Lokalisierung bleibt davon unabhängig.

Voreinstellungen sind deutsche Audiospurpräferenz, Untertitel aus und `hwdec=auto-safe`. Die aktive Bildausgabe dieser Stufe ist jedoch Software-Rendering; daraus folgt keine Abnahme von Hardware-Decodierung oder GPU-/HDR-Ausgabe. Der direkte GPU-Pfad bleibt eine eigene offene Integrationsaufgabe. Für ihn wurden in dieser Umgebung schwarze Bilder/Rasterartefakte beobachtet; er ist nicht Teil dieses Builds.
