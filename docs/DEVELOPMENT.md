# Entwicklungsprotokoll · 29.10.2026

## Grundlage und Freigaben

Das Gesamt-Pflichtenheft wurde vor Implementierungsbeginn vollständig gelesen. Der aktuelle Auftrag erteilt die ausdrückliche Entwicklungsfreigabe für das technische Fundament; die älteren Planungsstatusangaben im PDF sind damit für diesen Auftrag überholt. Die übrigen Anforderungen bleiben in REQUIREMENTS.md erhalten.

Das vorhandene öffentliche Repository TimBooH/VELUNO war leer. Der Nutzer erlaubt ausdrücklich einen leeren Initial-Commit für `main`. Dieser leere Stand wurde lokal als `main` angelegt; alle Implementierungsänderungen liegen ausschließlich auf `unstable`. Bis zur gesonderten Upload-Freigabe wurden keine Quellen hochgeladen und keine Actions auf GitHub ausgeführt. Releases bleiben gesperrt; die anschließende GitHub-Prüfung ist unten dokumentiert.

## Schritt 1 · Struktur und Build

C++20, CMake ≥ 3.22 und Qt ≥ 6.4 eingerichtet; getrennte Targets für Datenbank, Kern und libmpv-Adapter sowie Schnittstellen-Targets für Scanner und Metadaten. Alle Modulorte aus Abschnitt 11.2 sind angelegt und dokumentiert. QML und Ressourcen sind in die ausführbare Datei eingebettet; Linux-Desktopdatei und vorläufiges Symbol sind installierbar.

Prüfung: Debug-Build mit GCC 14.2, CMake 3.31.6, Qt 6.8.2 und libmpv-Client-API 2.5 unter Debian 13 erfolgreich. CMake-Installation in einen separaten Prefix und Start daraus erfolgreich. Entwicklungswerkzeuge wurden für diese eingeschränkte Umgebung unter `/tmp/veluno-sdk` entpackt; diese Besonderheit ist keine Projektvoraussetzung.

## Schritt 2 · SQLite und Einstellungen

Migration 001 erstellt Standardprofil und App-Einstellungen. Migrationen sind versioniert und transaktional. Konsistenzprüfung und Ablehnung neuerer Schemata verhindern stilles Überschreiben. Das Design wird persistent gespeichert; ein Schreibfehler lässt das bisherige Design bestehen und erscheint in den Einstellungen.

Prüfung: Fünf Datenbank-Testfälle erfolgreich: Erststart/Neustart/Persistenz/kein doppeltes Profil, Rollback bei fehlerhafter Migration mit erfolgreichem Folgelauf, unveränderte neuere Datenbank, unveränderte defekte Datenbank, gesperrte Datenbank mit anschließender Erholung. Der Lock-Test wartet absichtlich auf das 5-Sekunden-Busy-Timeout.

## Schritt 3 · Desktop-Oberfläche

Reguläres Qt-Quick-Fenster mit Systemdekorationen; Midnight Blue als Standard und gespeicherter Light Mode. Feste obere Navigation mit den fünf vereinbarten Bereichen; Suche rechts als noch deaktivierter Platzhalter. Einstellungen ausschließlich über das Profilmenü erreichbar. Leere Startseitenreihen sind nicht sichtbar. Die sieben Einstellungsbereiche sind angelegt; nur das Erscheinungsbild ist bereits funktional.

Prüfung: Drei QML-Testfälle für Navigation, Profilmenü/Einstellungen/Theme-Auswahl per Tastatur sowie Fenstergröße/Schließen erfolgreich, jeweils offscreen und mit nativem Wayland-Backend gegen Weston 14 im Headless-Modus. Beide Designs und die Startseite wurden als echte Qt-Renderbilder geprüft; Aufnahmen liegen in `screenshots/`.

Befunde und Korrekturen: Die anfänglich zu breite Einstellungsnavigation wurde auf 180 px begrenzt und mit einer Breitenprüfung abgesichert. Der QML-Testaufbau initialisiert SQLite erst nach Erzeugung der Anwendung und gibt Verbindungen vor deren Ende frei. Tastaturereignisse werden im Test ausdrücklich an das getestete Fenster zugestellt, damit die Aktivierungsreihenfolge des Wayland-Compositors den Test nicht verfälscht.

## Schritt 4 · Modulgrenzen

libmpv ist eine echte Build-/Link-Abhängigkeit. Der Adapter besitzt einen vorbereiteten RAII-Lebenszyklus, initialisiert aber keine Wiedergabe oder Geräte. Scanner und Metadaten sind ausschließlich Schnittstellen; kein Scan, kein Onlineabruf und keine Mediendateiänderung werden gestartet.

Prüfung: Alle Komponenten bauen zusammen. Die libmpv-API-Version wird beim Start erfolgreich gelesen. Eine Video-/Audio-Wiedergabe wird damit ausdrücklich nicht behauptet. Die Metadaten-Einwilligung bleibt unentschieden, es existiert kein Netzwerkclient.

## Schritt 5 · Paket und CI

Actions-Workflow nur für `unstable`, mit Tests vor Paketierung/Artefakt-Upload und ohne Release-Schritt. Paketierungswerkzeuge sind per SHA-256 fixiert. Lokales AppImage wurde gebaut und mit isolierten Qt-/Bibliothekspfaden offscreen gestartet.

Befund und Korrektur: Der erste Paketversuch nutzte unbemerkt Plugins aus der Build-Umgebung. Die isolierte Prüfung deckte fehlende Offscreen-/Wayland-Plattformplugins auf. Diese werden nun ausdrücklich paketiert; die isolierte Paketprüfung ist Bestandteil des Skripts. Das korrigierte Paket startet ohne die SDK-Pfade erfolgreich offscreen. Ergebnis des abschließenden nativen Wayland-Paketstarts: siehe Abschlussprüfung unten.

## Verbleibende Grenzen und offene Prüfungen

- Kein CachyOS/KDE-Plasma-Desktop in dieser Umgebung: tatsächliches Fensterverschieben, KWin-Dekorationen, Mehrschirmbetrieb und HiDPI sind nicht abgenommen. Weston bestätigt nur den nativen Wayland-Start und die automatisierte Bedienung mit Software-Rendering.
- Kein GPU-/HDR-/Video-/Audio-Test; die konkrete mpv-Renderintegration bleibt O-003/O-004.
- AppImage wurde lokal mit Extraktion statt FUSE gestartet. Die Ubuntu-Actions-Ausführung ist inzwischen erfolgreich abgeschlossen (Nachweis unten); die Auslieferung auf einem frischen Zielsystem steht aus. Das lokale Paket wurde auf Debian 13 gebaut und ist keine zugesagte Distributionsmatrix.
- CMake meldete in der entpackten Qt-Umgebung fehlende importierte QML-Plugin-Targets; die dynamischen Plugins wurden zur Laufzeit und im Paket erfolgreich geladen. Diese Umgebungswarnungen wurden nicht durch Ausblenden unterdrückt.
- linuxdeploy meldete fehlende Copyright-Dateien für einzelne entpackte Abhängigkeiten, die der System-Paketdatenbank nicht bekannt sind. Drittanbieterhinweise und Lizenzen sind vor einer Veröffentlichung vollständig zu prüfen. Keine Releasefreigabe wird daraus abgeleitet.
- Einrichtung, Medienimport, Suche, echte Profilverwaltung, Wiedergabe, Favoriten/Sammlungen, Sicherung und Updates bleiben gemäß Auftrag spätere Schritte.
- Bei einem Datenbankfehler erscheint derzeit eine Startdiagnose auf stderr; ein grafischer Reparaturdialog ist noch nicht implementiert. Die Anwendung ersetzt keine beschädigten Daten.

## Lokale Abschlussprüfung vor Upload

Korrigiertes AppImage erfolgreich nativ unter Wayland gestartet (`platform: wayland`, Exit 0), ohne `LD_LIBRARY_PATH`, `QT_PLUGIN_PATH`, `QML_IMPORT_PATH` oder `QML2_IMPORT_PATH` aus der SDK-Umgebung. Auch der isolierte Offscreen-Start ist erfolgreich. Paketgröße ungefähr 120 MiB; Prüfsummendatei liegt neben dem lokalen Paket im ignorierten Buildverzeichnis.

Zu diesem lokalen Prüfstand waren die Quelländerungen auf `unstable` gesichert; `main` enthielt ausschließlich den freigegebenen leeren Initial-Commit. Ein GitHub-Upload und Actions-Artefakt-Nachweis standen zu diesem Zeitpunkt noch aus. Die anschließende Freigabe und CI-Prüfung sind im folgenden Abschnitt dokumentiert.

## GitHub-Freigabe und erster CI-Lauf

Nach ausdrücklicher Upload-Freigabe wurden der leere `main`-Stand und `unstable` übertragen. Lauf [40937803490](https://github.com/TimBooH/VELUNO/actions/runs/40937803490) baute unter Ubuntu 24.04 / Qt 6.4.2 erfolgreich; Datenbanktests, Profilmenü/Theme und Fensterprüfung bestanden. Der Navigationstest scheiterte an `for…of` über die QML-Kinderliste, deren Iterator Qt 6.4 noch nicht bereitstellt. Die Traversierung nutzt nun kompatiblen Indexzugriff; kein Anwendungscode wurde dafür geändert. Das vom QML-Importscanner vermisste Paket `qml6-module-qtqml` ist in den Abhängigkeiten ergänzt.

## Erfolgreicher Actions-Nachweis

Lauf [40938184412](https://github.com/TimBooH/VELUNO/actions/runs/40938184412) für Commit `7373f0c54c2f592956580cdcbf2ada5a46cbd299` ist vollständig erfolgreich: Ubuntu 24.04, Qt 6.4.2, Build, beide Testsuiten, Programmstart, AppImage-Paketierung, isolierter Paketstart und Artefakt-Upload.

[Artefakt 12924389858](https://github.com/TimBooH/VELUNO/actions/runs/40938184412/artifacts/12924389858) enthält AppImage und SHA-256-Prüfsumme; ZIP-Größe 112.192.616 Bytes. Aufbewahrung 14 Tage, Ablauf laut GitHub am 12.11.2026 um 20:16 Uhr Europe/Berlin. ZIP-SHA-256: `d549ddb3b9475ecabba73f71ffcdc201822706222f93ff47d735d610b2c2ac5a`.

Nicht blockierende CI-Hinweise: linuxdeploy überspringt erneutes Strippen bereits bearbeiteter ELF-Dateien; AppStream-Metadaten sind noch nicht angelegt; die verwendeten Actions melden Node-20-/Node-24-Übergangshinweise. Diese Hinweise verhindern weder erfolgreichen Paketstart noch Upload. Die Zielsystem-/GPU-/HiDPI-Abnahme bleibt ausstehend.

`main` bleibt bei `864b7861add9787915cb714fc7bdf1260167b606` (leerer Initial-Commit). Kein Stable-Stand und kein GitHub-Release wurden erstellt. Der anschließende reine Dokumentationscommit überspringt CI mit `[skip ci]`; der geprüfte Quell-/Paketstand ist der oben verlinkte Commit.

## Nächster Entwicklungsschritt · Grundlegende lokale Wiedergabe

Nach erfolgreichem Download gab der Nutzer die Fortsetzung frei und wählte zuletzt ausdrücklich die grundlegende libmpv-Wiedergabe. Ordnerverwaltung und Scanner werden separat weitergeführt.

Umgesetzt: native Dateiauswahl/Strg+O, integriertes Bild, Pause/Fortsetzen, Zeitleiste und ±10 Sekunden, Lautstärke/Stumm mit Speicherung beim Beenden, Geschwindigkeit 0,5×–2×, Vollbild/Esc, Audio-/Untertitelspur-Kürzel und sichtbare Wiedergabefehler. Das Overlay blendet nach zwei Sekunden aus und bleibt bei Pause sichtbar.

Der zunächst untersuchte direkte OpenGL-Pfad zeigte instabile schwarze Bilder und Rasterartefakte. Er wurde nicht in den finalen Stand übernommen. Die erste funktionale Stufe verwendet die offizielle libmpv-Software-Render-API auf einem eigenen Worker-Thread mit begrenztem Einzelbildpuffer. GPU-/Hardware-/4K-/HDR-Abnahme bleiben ausdrücklich offen. Details und verbleibender Funktionsumfang: [PLAYER.md](PLAYER.md).

Lokale Abschlussprüfung unter Debian 13 / Qt 6.8.2: alle vier CTest-Suiten bestanden (`player`, `player_render`, `database`, `qml`). Der native Wayland-Test nutzt Weston mit Software-Compositor und kontrolliert mindestens 98 % der gewählten roten/blauen Bildregionen, Geometrieänderung und Abbau während aktiver Wiedergabe. Zusätzlich bestand der identische Bildtest unter X11/Xvfb mit der finalen Softwareausgabe. Damit gilt die frühere X11-Einschränkung des verworfenen OpenGL-Adapters nicht als Einschränkung dieses finalen Software-Adapters. Echte Audioausgabe, CachyOS/KDE, GPU-Decodierung und HiDPI sind weiterhin nicht auf Zielhardware geprüft.

Die Tests verwenden ausschließlich generierte Testmedien und vergleichen Originaldateien per SHA-256. Fehlerfälle (fehlender Pfad, nicht lokale URL, beschädigtes Video) mit anschließender Erholung sowie wiederholtes Öffnen/Stoppen sind geprüft. Die QML-Suite prüft die Player-Bedienung und Overlay-Regeln zusätzlich zur bisherigen Navigation und Theme-Persistenz.

Erster Player-CI-Lauf [40949224547](https://github.com/TimBooH/VELUNO/actions/runs/40949224547): Build, Controller-, Datenbank- und nativer Wayland-Bildtest bestanden unter Ubuntu 24.04 / Qt 6.4.2. Der QML-Test deckte auf, dass Qt 6.4 beim erstmaligen Hineinbewegen nur das MouseArea-Eintrittsereignis liefert. Das Overlay reagiert nun zusätzlich auf `onEntered`; das ist eine Korrektur der echten Bedienung, keine Abschwächung der Testprüfung.

Abgeschlossener Player-CI-Nachweis: [Lauf 40949745517](https://github.com/TimBooH/VELUNO/actions/runs/40949745517) für `3f5db6f7c0eb0566c86527b24eead12d61a14254`, Ubuntu 24.04 / Qt 6.4.2 / libmpv-API 2.2. Alle vier Testsuiten bestanden; Programmstart, AppImage-Paketierung, isolierter Paketstart und Upload waren erfolgreich.

[Player-Artefakt 12930812787](https://github.com/TimBooH/VELUNO/actions/runs/40949745517/artifacts/12930812787) enthält AppImage und Prüfsumme, ZIP-Größe 113.299.587 Bytes. Ablauf laut GitHub am 12.11.2026 um 21:35 Uhr Europe/Berlin. ZIP-SHA-256: `39de1fe2f95621958c48d065615377bfe184c36586a6be557aecf5c7de5ba1fc`. Das Paket wurde isoliert gestartet; die Wiedergabe echter Medien im AppImage auf CachyOS mit echten Audiogeräten steht noch zur Nutzerabnahme aus. AppStream-Metadaten und die bereits bekannten Actions-Laufzeithinweise bleiben nicht blockierende Distributionshinweise.

Dieser Abschnitt wird mit einem reinen Dokumentationscommit `[skip ci]` ergänzt. `main` bleibt auf dem leeren Initial-Commit; es wurde kein GitHub-Release erstellt.


## 30.10.2026 · Zusammenhängender Ausbau der lokalen Bibliothek

Der Nutzer hat die Fortsetzung freigegeben und die ursprüngliche Vorgabe kleiner einzelner Entwicklungsschritte aufgehoben. Dieser Stand bündelt Quellenverwaltung, Hintergrundscan und Bibliotheksoberfläche mit der bestehenden Wiedergabe. Entwicklung bleibt ausschließlich auf `unstable`; es gibt keine Stable-/Releasefreigabe.

Umgesetzt: mehrere Film-/Serienordner, rekursive Discovery, ffprobe-Analyse, lokale Titel-/Episodenerkennung, manuelle Zuordnung, reale Versionsauflösung, Offline-Erhalt, Dateibeobachtung mit 60-s-Rückfallprüfung, Fortschritt/Pause/Fortsetzen und persistente Scanfehler. Dazu Film-/Serienraster, Startseitenreihen, lokale Titelsuche, Version-/Episodenauswahl mit Playeranbindung, profilbezogene Ansichtseinstellungen, sechsstufiger Assistent und ausdrückliche Metadatenentscheidung. Migration 2 ergänzt die Tabellen ohne Änderung der initialen Migration; WAL und separate Worker-Verbindung vermeiden lange GUI-Schreibsperren. Details und Grenzen stehen in LIBRARY.md.

Während der Prüfung korrigiert: ein nuller QString verletzte die NOT-NULL-Diagnosespalte; der Parser benötigte zuvor existierende Elternordner; sortierte Dateimodelle mussten Zeilenbewegungen statt reiner Primärschlüsselreihenfolge unterstützen. Ein Suchtreffer zerstörte beim Leeren des Suchfelds seinen eigenen QML-Delegate-Kontext; die Aktivierung liegt nun im übergeordneten Suchfeld. Verfügbarkeitsfilter, Assistenten-Testzustand und manuelle Zuordnung wurden mit tatsächlicher UI-Bedienung geprüft. Fehler werden erst nach abgeschlossenem lesbarem Quellscan bereinigt und bleiben bei Abbruch erhalten.

Lokale Abschlussprüfung: C++-/QML-Build unter Qt 6.8.2 erfolgreich; alle fünf CTest-Suiten bestanden (`player`, `player_render`, `library`, `database`, `qml`), insgesamt 18,53 Sekunden. Der Wayland-Bildtest läuft mit Weston/Software-Compositor. Bibliothekstests nutzen synthetische 1920-/3840-Pixel-Medien, rekursive/überlappende Quellen, defekte/unklare Dateien, manuelle Korrektur, Pause/Fortsetzen/Neustart, Dateibeobachtung und simulierte Offline-/Rückkehrzustände; SHA-256 bestätigt unveränderte Medien. Die QML-Suite bedient Suche, Raster, Wiedergabe, Zuordnung und Assistenten. Screenshots zeigen ausschließlich synthetische Testeinträge.

Offen: Onlineanbieter/Cover, vollständige Detailinformationen, Sprach-/Avatarwahl, Favoriten/Sammlungen, Fortschritt/Gesehen, Backups/Updates sowie GPU/HDR-Abnahme. Native KDE-Ordnerdialoge und D-Bus-Zustellung, physische Wechseldatenträger/Netzlaufwerke, große Bibliotheken und HiDPI sind in dieser Umgebung nicht auf dem Zielsystem getestet. Die entpackte lokale SDK-Umgebung meldet weiterhin fehlende importierte QML-Plugin-Targets; der erfolgreiche Lauf verwendet dynamische Plugins. Kein Hinweis wird als Zielsystemabnahme gewertet.

Zusätzlich bestand der Programmstart mit Bibliotheksimport nativ unter Wayland (`platform: wayland`, ein erkannter Film, null Fehler, Exit 0), mit neuem Datenverzeichnis und Weston-Software-Compositor. Der entsprechende Offscreen-Importstart mit ebenfalls frischer Datenbank bestand ebenfalls. Eine anfangs ausgegebene Fontconfig-Meldung zur schreibgeschützten SDK-Cache-Umgebung verhinderte den Offscreen-Start nicht; mit explizitem schreibbarem Cache im Wayland-Test trat sie nicht auf.

## Bibliotheks-CI und Unstable-Artefakt

[Lauf 41069263260](https://github.com/TimBooH/VELUNO/actions/runs/41069263260) für Commit `021b99d37367c2220fd8c21601eb9a5ac02c69cc` bestand unter Ubuntu 24.04 / Qt 6.4.2: Build, alle fünf Testsuiten, Programmstart, AppImage-Paketierung, isolierter Import eines synthetischen Films mit dem paketierten ffprobe und Artefakt-Upload. Der Import meldete einen Film und null Fehler.

[Artefakt 12984037864](https://github.com/TimBooH/VELUNO/actions/runs/41069263260/artifacts/12984037864) enthält AppImage und Prüfsumme; ZIP-Größe 115.111.600 Bytes. Aufbewahrung 14 Tage, Ablauf laut GitHub am 13.11.2026 um 14:12:15 UTC. ZIP-SHA-256: `c99067a5eeffc2da2a54b52daa57c3505cfb520298d66333af583a8bebc03057`.

Nicht blockierende Paketierungshinweise bleiben: bereits bearbeitete ELF-Dateien werden nicht erneut gestrippt, AppStream-Metadaten fehlen und die Paketierungswerkzeuge melden veraltete Ausgabebezeichner; Actions meldet Node-API-Abkündigungen. Die isolierte Paketprüfung und der Upload bestanden. CachyOS/KDE, FUSE und reale Medien-/Hardware-Abnahme bleiben offen.

`main` wurde remote als unveränderter leerer Initial-Commit `864b7861add9787915cb714fc7bdf1260167b606` geprüft. Es wurde kein GitHub-Release erstellt. Dieser Nachweis wird als reiner Dokumentationscommit mit `[skip ci]` ergänzt.

### Ergänzende Dialog-/Paketprüfung

Die Detailprüfung des erfolgreichen Paketierungsprotokolls meldete `QtQml.Models` und `QtQuick.Shapes` als fehlend. Der geprüfte Importpfad benötigte diese noch nicht, Qt-Dialog-Fallbacks können sie jedoch verwenden. Beide QML-Laufzeitpakete sind jetzt ausdrücklich in den CI-/Debian-Abhängigkeiten enthalten. Der Assistententest öffnet und schließt zusätzlich den tatsächlichen Qt-Ordnerdialog; die erweiterte QML-Suite bestand lokal. Die native KDE-Portalvariante bleibt separat abzunehmen. Meldungen des statischen Scanners zu Windows/macOS/iOS-Stilen sowie zum in C++ registrierten `Veluno.Player` werden von echten fehlenden Linux-Laufzeitmodulen unterschieden.
