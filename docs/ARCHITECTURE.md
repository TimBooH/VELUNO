# Architektur

Die Anwendung ist ein natives Qt-Quick-ApplicationWindow. Qt wählt Wayland aus der Desktopsitzung; Fensterdekorationen und Vollbild bleiben beim Fenstersystem. QML kennt ausschließlich QObject-Fachcontroller, keine SQLite-Verbindungen und keine mpv-Handles.

| Modul | Verantwortung |
| --- | --- |
| app/core | Lebenszyklus, Instanz-Lock, Theme, Sprache und Verdrahtung |
| database | Gebundene SQL-Parameter, Transaktionen, Migrationen 1–4, WAL und Repository |
| library/scanner | Gemeinsame Film-/Serienbibliothek, Quellen, Parser, ffprobe, inkrementelle Modelle und Beobachtung |
| profiles/collections | PersonalController für Profile, private Favoriten/Sammlungen, Fortschritt, Gesehen und Episodenwechsel |
| metadata | Optionaler TMDB-Controller, Zustimmung, gedrosselte asynchrone Abfragen, Korrekturen und Bildvarianten |
| player | libmpv-Session, asynchrone Befehle, Spuren/Geräte/Kapitel, Einstellungen und separate Videoausgabe |
| backup | Konsistente Snapshots, referenzierte Bilder, Prüfung, Rotation und vorbereitete Gesamtwiederherstellung |
| updater | Konfigurierbare signierte Manifeste, verifizierter Download und ausdrücklich bestätigter atomarer Austausch |
| platform | KDE-KIO-FUSE über D-Bus, dauerhafte SMB-Identitäten und temporäre lokale Zugriffspfade |
| qml | Navigation, Detaildialoge, persönliche Seiten, Einstellungen, Assistent, Themes und Player |

## Daten und Threads

SQLite-Verbindungen gehören ihrem erzeugenden Thread. Der Scanner besitzt eine eigene Verbindung auf seinem QThread. Ergebnisse erreichen die GUI als queued snapshots und werden über Zeilenänderungen/-bewegungen statt vollständigem Modellreset eingearbeitet. Kurze Schreibtransaktionen speichern Dateien einzeln; Discovery und ffprobe halten keine lange Schreibtransaktion offen. GUI-Controller greifen auf die GUI-Verbindung zu.

Persönliche Tabellen sind über profile_id getrennt. Medienquellen und Titel bleiben gemeinsam. Fortschritt verwendet Profil/Titel/Staffel/Episode als Identität; Filmversionen teilen ihren Fortschritt. Ein Profilwechsel speichert den aktuellen Stand und stoppt mpv vor dem Wechsel des aktiven Profils.

Metadaten verwenden QNetworkAccessManager ohne blockierendes Warten. Ohne API-Zugang und ausdrückliche Zustimmung wird kein Titelrequest gestartet. Bilder werden auf einem separaten Worker decodiert und atomar gespeichert. Manuelle Feld-/Bildentscheidungen haben persistente Schutzflags. Der Provider ist derzeit TMDB; Schnittstelle und Modul sind getrennt, Anbieterbedingungen/Attribution und weitere Anbieter bleiben eigenständig zu prüfen.

Sicherungen erzeugen auf ihrem Worker über VACUUM INTO einen konsistenten DB-Snapshot und kopieren ausschließlich referenzierte dauerhafte Bilder. Wiederherstellung wird nach Prüfung und Vorsicherung vorbereitet und vor Öffnen der DB beim Neustart aktiviert. Ein außerhalb des Datenordners liegender Instanz-Lock verhindert konkurrierende Anwendung bei diesem Austausch. Details und Fehlergrenzen: FULL_BUILD.md.

Updateprüfung/Download sind asynchron. Das Ed25519-signierte Manifest ist die Vertrauensbasis; reine HTTPS-/Prüfsummenaussagen reichen nicht als Ersatz. Installation erfordert explizite Aktion und einen erfolgreichen Update-Snapshot. Kopieren/Verifizieren/Austausch laufen auf einem Worker, die alte AppImage-Datei bleibt erhalten. Veröffentlichung wird dadurch nicht autorisiert.

## Videoausgabe

MpvEngine besitzt die geteilte mpv-Session und leert Ereignisse alle 30 ms ohne blockierendes Warten. Befehle sind asynchron; Benutzer-MPV-Konfiguration und externe Skripte sind deaktiviert. VideoWorker besitzt den offiziellen libmpv-Software-Renderkontext auf einem eigenen Thread. Ein begrenzter Einzelbildpuffer hält das neueste Frame; höchstens eine GUI-Benachrichtigung wartet gleichzeitig. VideoItem zeichnet das Bild über QQuickPaintedItem. Der Renderkontext wird auf seinem Thread freigegeben, bevor Session und mpv beendet werden.

Hardware-Decodierung ist konfigurierbar; die aktive Software-Bildausgabe ist kein HDR-/GPU-/4K-Leistungsnachweis. Der zuvor untersuchte fehlerhafte OpenGL-Adapter wurde nicht wieder eingeführt. Benannte Abspannkapitel, manuelle Marker und Ende-Fallback steuern den Seriencountdown.

## Persistenz und Paket

Migrationen laufen atomar und lückenlos. Fremdschlüssel, Busy-Timeout, quick_check und WAL sind aktiv; neuere Schemata werden abgewiesen. Die Anwendung repariert oder ersetzt beschädigte Daten nicht stillschweigend. Tokens liegen in der besitzerbeschränkten DB; private Backups enthalten sie. Eigene Bilder werden kopiert, Originalmedien nie umbenannt/verschoben/gelöscht.

AppImage enthält Qt/QML-/Wayland-/SQLite-/TLS-Plugins, libmpv, ffprobe und OpenSSL-Laufzeitbibliotheken. Isolierte Pakettests entfernen SDK-Pfade und prüfen TLS-Backend, Import und Dialog. KIO-FUSE/KDE-Sitzung, Systemzertifikate und die echte Zielhardware bleiben Hostvoraussetzungen. Quell- und Paketnachweise stehen im Entwicklungsprotokoll; die Statusmatrix unterscheidet Implementierung von Abnahme.
