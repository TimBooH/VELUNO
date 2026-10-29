# Grundlegende lokale Wiedergabe · 29.10.2026

## Freigegebener Schritt

Der Nutzer hat nach dem getesteten Projektfundament die Fortsetzung freigegeben und zuletzt ausdrücklich „Grundlegende Videowiedergabe mit libmpv“ gewählt. Die zuvor ebenfalls gewählte Ordner-/Scan-Stufe wird anschließend umgesetzt. Entwicklung bleibt auf `unstable`; keine Stable-Freigabe.

## Verhalten

Lokale Datei über Startseite, Strg+O oder Profilmenü öffnen; integriertes Bild im bestehenden Fenster, Pause/Fortsetzen, Zeitleiste, ±10 Sekunden, Lautstärke/Stumm, 0,5×–2×, Vollbild, grundlegende Audio-/Untertitelspur-Kürzel. Das Overlay blendet nach zwei Sekunden aus und bleibt bei Pause sichtbar. Neue Dateien starten ohne Untertitel. Deutsche Audiospuren sind bevorzugt. Navigieren, Zurück und Schließen beenden die Wiedergabe. Die letzte Lautstärke wird bei regulärem Beenden gespeichert.

Es werden keine Originaldateien umbenannt, verschoben oder gelöscht. Der gewählte Dateipfad wird nur zum Lesen an mpv übergeben; die Tests vergleichen die Mediendatei vor und nach Wiedergabe per SHA-256.

## Technische Grenze dieser Stufe

Die direkte OpenGL-Integration wurde untersucht, zeigte in der Cloud-Umgebung jedoch schwarze Frames und Rasterartefakte. Sie ist nicht aktiviert und wird nicht als erledigt ausgegeben. Die erste funktionale Wiedergabestufe verwendet stattdessen die offizielle libmpv-Software-Render-API mit separatem Worker und begrenzter Bildübergabe an Qt Quick. Dies ist eine Zwischenstufe, keine Streichung der GPU-/Hardwareanforderungen.

CPU-Last und erreichbare Bildrate hängen von Auflösung und Hardware ab. 4K/HDR, echte GPU-Decodierung, Farbmanagement und längere Wiedergabesitzungen sind nicht abgenommen. Es gibt noch keine Wiedergabefortschritts-Persistenz, Gesehen-Markierung, Filmversionsauswahl, nächste Episode, Gerätewahl, Untertitelgestaltung oder frei belegbare Kürzel. Diese Punkte bleiben im Pflichtenheft erhalten.

## Prüfungen

- Synthetische MPEG-4/AAC-Datei mit Unicode und Leerzeichen im Pfad: Laden, Fortschritt, Pause, absolute/relative Sprünge, Grenzwerte für Lautstärke/Geschwindigkeit, Stumm, Wiedergabeende, Neustart, wiederholtes Öffnen/Stoppen und unveränderte Dateiprüfsumme.
- Fehlender Pfad, nicht lokaler Direktpfad und beschädigte Datei: sichtbare Fehler und anschließendes erfolgreiches Öffnen einer gültigen Datei.
- Nativer Wayland-Bildtest: bekannte rote/blaue Flächen werden über mindestens 98 % der geprüften Bildregionen nachgewiesen; Größenänderung und Abbau während aktiver Wiedergabe sind Teil des Tests.
- QML-Bedienung: echtes Klicken auf Pause und Zeitleiste, zwei Sekunden Overlay-Ausblendung, sichtbare Controls bei Pause, Stumm-Kürzel, Vollbild/Esc, Zurück/Stop; bisherige Navigation, Themes und Fenster bleiben geprüft.
- Ton wird in den Controller-Tests über einen Null-Ausgang geprüft; das Render-Testvideo ist stumm. Echte PipeWire-/Audio-Geräte und hörbare Wiedergabe sind in der Cloud nicht geprüft.

Finaler Nachweis: alle vier Testsuiten und Paketstart erfolgreich in [Actions-Lauf 40949745517](https://github.com/TimBooH/VELUNO/actions/runs/40949745517). [AppImage-Artefakt](https://github.com/TimBooH/VELUNO/actions/runs/40949745517/artifacts/12930812787). Details stehen im Entwicklungsprotokoll. Die KDE-/CachyOS-/HiDPI-Abnahme bleibt auf dem Zielsystem erforderlich.
