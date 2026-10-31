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

Der erweiterte Dialogtest scheiterte im CI-Lauf [41070055732](https://github.com/TimBooH/VELUNO/actions/runs/41070055732) unter Qt 6.4: die Qt-Fallbackimplementierung konnte `Qt.labs.folderlistmodel` nicht laden. Ein Artefakt wurde folgerichtig nicht erzeugt. Die zusätzliche Laufzeitabhängigkeit ist nun installiert und in FolderPanel ausdrücklich importiert, damit der AppImage-QML-Scanner sie auch ohne Analyse interner Qt-Ressourcen mitnimmt. Die Testprüfung wurde beibehalten.

Die isolierte AppImage-Prüfung lädt zusätzlich über `--smoke-test --smoke-dialog` den tatsächlichen Ordnerdialog, verlangt seine Sichtbarkeit und schließt ihn wieder. Dieser separate Pakettest verhindert, dass fehlende Dialogmodule hinter einem erfolgreichen Import-API-Test verborgen bleiben. Lokal bestanden der Dialog-Smoke-Test und die erweiterte QML-Suite nach dieser Ergänzung.

### Abgeschlossener Nachweis einschließlich Ordnerdialog

[Lauf 41070627636](https://github.com/TimBooH/VELUNO/actions/runs/41070627636) für `0f2c2efba0ae96e693f8c5cfaaa89af5ab67c73e` ist vollständig erfolgreich. Ubuntu 24.04 / Qt 6.4.2: alle fünf Testsuiten, Programmstart, Paketierung, isolierter Medienimport (`true`, ein Film, null Fehler), isolierter Ordnerdialog (`true`) und Artefakt-Upload bestanden. Die zuvor fehlenden Linux-QML-Module werden nicht mehr gemeldet. Übrig bleiben statische Scanner-Meldungen zu fremden Plattformstilen und dem in C++ registrierten `Veluno.Player`; die tatsächliche Laufzeitprüfung ist erfolgreich.

[Aktuelles Artefakt 12983369990](https://github.com/TimBooH/VELUNO/actions/runs/41070627636/artifacts/12983369990): AppImage und Prüfsumme, ZIP-Größe 115.239.822 Bytes, Ablauf am 13.11.2026 um 14:22:32 UTC. ZIP-SHA-256: `bfd86db40cba2563afb7f196c3b6c8387d4136928b5f81923842085008cffb7b`. Dieser Nachweis ersetzt den früheren Paketstand für die aktuelle Nutzung; ältere Läufe bleiben als Verlauf dokumentiert. Die CachyOS-/KDE-/Hardware-Abnahme bleibt offen. Kein Stable-Release, keine Änderung an `main`.

## 30.10.2026 · SMB-Zugriff nach Nutzerfehlerbericht

Der Nutzer meldete fehlenden Zugriff aufs Heimnetz und bestätigte SMB als Protokoll. Ursache: die Quellenverwaltung akzeptierte ausschließlich lokale file-URLs; Qt allein stellt keine rekursiven SMB-Dateizugriffe bereit. Ergänzt wurden ein SMB-Adressdialog und die KDE-KIO-FUSE-Anbindung über den dokumentierten mountUrl-D-Bus-Vertrag. Quellen/Dateien behalten dauerhafte SMB-URLs; Scan und Player verwenden jeweils frisch aufgelöste lokale Pfade. Das benötigt keine Schemaänderung und erhält manuelle Zuordnungen bei geänderten Einhängepfaden. Zugangsdaten werden in Adressen abgelehnt; Anmeldung übernimmt die vorhandene KDE-/Dolphin-Integration. KIO-FUSE und kio-extras sind Hostabhängigkeiten, keine mitgepackten Dienste.

Verbindungsanfragen laufen asynchron bzw. im Scanner-Worker. Ausstehende Wiedergaben werden beim Abbruch verworfen. Für bereits aufgelöste Netzwerkdateien entfällt die sonstige Dateisystem-Vorprüfung auf dem GUI-Thread; mpv meldet Lesefehler asynchron. SMB-Beobachtung erfolgt durch die vorhandene 60-s-Prüfung statt unzuverlässiger inotify-Ereignisse. D-Bus-Wartezeit ist auf 30 Sekunden begrenzt; bereits laufende FUSE-Dateisystemoperationen sind weiterhin vom Hosttreiber abhängig.

Neue Netzwerksuite mit privatem D-Bus und nachgebildetem KIO-FUSE: URL-Validierung, keine gespeicherten Zugangsdaten, fehlender Dienst, Import, echte ffprobe-Analyse, Unicode-Pfade, manuelle Zuordnung, Wechsel des Mountpfads ohne Duplikate, Offline/Rückkehr, abgebrochene Wiedergabe und libmpv-Anbindung. Die QML-Suite öffnet zusätzlich den SMB-Dialog. Beim Testaufbau wurde ein versehentlich konstanter QDBusConnection-Testhandle korrigiert, bevor die Netzwerksuite ausführbar war.

Testgrenze: In der Cloud fehlen `/dev/fuse`, eine KDE-Sitzung und Zugriff auf das Nutzer-NAS. Echter SMB-Transport, Authentifizierung und Netzwerkverlust während tatsächlicher NAS-Wiedergabe sind nicht abgenommen. Die neue Schnittstelle ist automatisiert getestet, kein behaupteter End-to-End-NAS-Nachweis. Netzwerk-Gerätesuche und eigene Passwortverwaltung sind nicht Teil dieses Fixes.

Lokaler Abschlussstand: Build erfolgreich; alle sechs CTest-Suiten (`player`, `player_render`, `library`, `network`, `database`, `qml`) bestanden in 18,91 Sekunden. Der native Wayland-Bildtest nutzt weiterhin Weston/Software-Compositor. Der neue Netzwerkpfad wurde zusätzlich bis in den echten libmpv-Controller geprüft, dessen I/O im Test von einer lokalen Datei hinter dem simulierten Mount stammt.

### Korrektur der lokalen Prüfung und des CI-Testaufbaus

Der erste SMB-CI-Lauf [41089241970](https://github.com/TimBooH/VELUNO/actions/runs/41089241970) stoppte beim Kompilieren der Netzwerksuite: `MpvEngine(true)` übergab bool an den QObject-Elternparameter. Richtig ist `MpvEngine(nullptr,true)`. Dies betrifft den Testaufbau und ist kein Qt-Versionsunterschied.

Die vorherige lokale Befehlsfolge stoppte nach dem Buildfehler nicht und führte alte Testprogramme aus. Die oben genannten 18,91 Sekunden belegen deshalb nicht die letzte Player-Test-Erweiterung. Das wird hier ausdrücklich korrigiert. Nach Korrektur des Konstruktors wurden Build und Tests mit `set -e` erneut ausgeführt: Neubau erfolgreich, alle sechs Testsuiten bestanden in 18,76 Sekunden, einschließlich des jetzt neu gebauten Netzwerk-/libmpv-Tests. Der Actions-Workflow hatte den Buildfehler korrekt erkannt und kein Artefakt erstellt.

### Erfolgreicher SMB-CI-Stand

[Lauf 41089751941](https://github.com/TimBooH/VELUNO/actions/runs/41089751941) für `8a5730a7e2b965f46de4ff6c8d668f3c658391c2` ist erfolgreich: Ubuntu 24.04 / Qt 6.4.2, Build, alle sechs Testsuiten, Programmstart, AppImage-Paketierung, isolierter Import/Ordnerdialog und Artefakt-Upload bestanden. Die Netzwerksuite verwendet den dokumentierten KIO-FUSE-Vertrag auf einem privaten Testbus; echte NAS-/FUSE-/Anmeldetests bleiben ausdrücklich ausstehend.

[SMB-Artefakt 12993657178](https://github.com/TimBooH/VELUNO/actions/runs/41089751941/artifacts/12993657178): ZIP mit AppImage und Prüfsumme, 115.449.772 Bytes. Ablauf am 13.11.2026 um 17:30:24 Uhr Europe/Berlin. ZIP-SHA-256: `293ddfcd1a8c8e2aaa69ea362c7c51e6ed24825ec68620cb1f663ee37711481e`. Die bekannten nicht blockierenden Paket-/Scannerhinweise gelten weiterhin. Kein GitHub-Release und keine Änderung an `main`; dieser reine Nachweiscommit überspringt CI.

## 30.10.2026 · Gesamtauftrag: fehlende Softwarebereiche vervollständigen

Der Nutzer hat ausdrücklich den vollständigen weiteren Funktionsausbau verlangt. Nach Rückfrage sollen TMDB-Zugang und Updatequelle später in den Einstellungen hinterlegt werden. Das ist keine Stable-/main-Freigabe. Das Pflichtenheft wurde erneut vollständig gelesen; sämtliche IDs bleiben in REQUIREMENTS.md nachvollziehbar.

Zusammenhängend ergänzt: mehrere lokale Profile und Avatare, private Favoriten/Sammlungen/Ansichtsoptionen, Poster-Collagen/eigene Bilder, Fortschritts- und Gesehen-Daten mit 90-%-Regel, Weiterschauen, benannte Abspannkapitel/manuelle Marker/Ende-Fallback und Episoden-Countdown. Player-Geräte-/Spurwahl, externe Untertitel und Darstellung, Decoderwahl sowie Konfliktprüfung/persistente Kürzel einschließlich N sind erreichbar. Der Assistent und Allgemein bieten eine gespeicherte Deutsch-/Englisch-Auswahl. Technische Meldungen können weiterhin deutsch sein.

Metadaten: optionaler eigener TMDB-Bearerzugang PLUS ausdrückliche Zustimmung; ohne beide Voraussetzungen keine Titelabfrage. Automatische eindeutige Zuordnung, manuelle Trefferwahl/Korrektur, lokal gespeicherte Bildvarianten und Schutz manueller Felder/Bilder. Bildverarbeitung auf Worker, Requests gedrosselt/asynchron, 429-Wartezeit. Die globale Suche bleibt gemäß SEARCH-001 ausdrücklich auf lokale Filme und Serien begrenzt; eine ältere Dokumentationsformulierung „Episoden/Sammlungen folgen“ wurde korrigiert.

Sicherungen: konsistenter SQLite-Snapshot, vollständige referenzierte Bilder, SHA-256-/SQLite-/Fremdschlüssel-/Vollständigkeitsprüfung, Sieben-Tages-Rotation sowie dauerhafte manuelle/Updatekopien. Gesamtwiederherstellung nach Vorsicherung beim Neustart mit zurückgelegtem Datenordner; Instanz-Lock verhindert parallele Aktivierung. Unbekannte Zusatzdateien in Sicherungsverzeichnissen verhindern automatisches Löschen, damit keine dort abgelegten fremden Originaldateien entfernt werden. Neue Migrationen 3 und 4 ergänzen persönliche Daten/Metadaten; 1 und 2 bleiben unverändert. Vor Migration wird ein eigener konsistenter Snapshot angelegt.

Updateclient: Stable Standard, konfigurierbare dauerhafte HTTPS-Quellen und öffentlicher Ed25519-Schlüssel; signierte Manifeste mit Kanal/Version/Build/glibc/Schema/Größe/SHA-256. Interner Download, explizite Installationsbestätigung, separate Update-Sicherung, nochmals verifizierte Bereitstellung im Zielordner und atomarer AppImage-Austausch mit erhaltener Vorgängerdatei. Signierwerkzeug erzeugt nur lokale Dateien, veröffentlicht nichts. Keine Updatequelle oder Schlüssel werden erfunden oder automatisch bereitgestellt. Buildinformationen sind als JSON abrufbar und werden dem Actions-Artefakt beigefügt.

Fehler während Entwicklung und Prüfung wurden korrigiert: fehlende Includes/Signalsignaturen und QML-Semikolons, eine Geräteabfrage im Null-Audio-Testmodus, eine Groß-/Kleinschreibung im HTTP-Header-Test (Headernamen sind nicht case-sensitiv), ein überstehender Profildialog und ein zunächst leerer Geräte-Selektor. Die Tests prüfen Suchtreffertext, reale Sprachumschaltung, Dialogbreite, Geräteanzeige und neue Seiten. Startfehler erhalten nun zusätzlich ein eigenes Desktopfenster statt ausschließlich stderr; Automatiktests beenden sich weiterhin mit Fehlercode.

Letzter kompletter lokaler Testlauf: alle acht CTest-Suiten bestanden in 24,64 Sekunden (`player`, `player_render`, `library`, `network`, `personal_backup`, `metadata_update`, `database`, `qml`). Darin sind externe Untertitel und Spurwahl, Profil-/Sammlungsisolation/Medien-Hashes, Fortschritt/90 %, Serienmarker/Countdown/Ende-Fallback, Kürzelkonflikte, alte Schemata vor Migration, acht tägliche Snapshots mit Siebenerrotation, Korruption und vollständige Rücksicherung, Schutz vor Zusatzdatei-Löschung, Metadaten-Einwilligung/geschützte Korrekturen, Signaturmanipulation/Kanal-/Schema-/glibc-Prüfung und atomarer Dateiaustausch enthalten. Der Wayland-Bildtest nutzt weiterhin Weston/Software-Compositor.

Die Formatierung wurde vereinheitlicht (.clang-format, qmlformat); die Werkzeuge wurden nur temporär außerhalb des Repositorys installiert. Alle geänderten Quellen sind überprüfbar. Eine zunächst ohne zusätzliche Netzwerk-/Unix-Socket-Freigabe gestartete echte Audio-Geräteabfrage im lokalen Smoke-Test blockierte im Pulse-Wakeup des Sandboxes; der begrenzte Starttest verwendet nun explizit Null-Audio, die normale App weiterhin reale Geräte. TLS-Backend und Qt-Dialog werden dabei geprüft. Daraus wird keine Geräteabnahme abgeleitet.

Nicht verfügbar bzw. nicht nachgewiesen: echte TMDB-Dokumentations-/API-Abnahme mit Nutzerzugang (Web-Dokumentationswerkzeug lieferte Fehler), aktuelle Anbieter-/Logo-/Bildrechte, reale signierte Hostingquelle, NAS/KDE-Anmeldung, hörbare Geräte, GPU/HDR/HiDPI/KWin und produktive Installation/Rückkehr auf Zielhardware. FULL_BUILD.md und O-001 bis O-010 nennen diese Grenzen. Kein Stable-Publish, keine Änderung an main und keine Behauptung vollständiger Zielsystemabnahme.

Zusätzliche Abschlussprüfungen: Der begrenzte Programmstart mit tatsächlich geöffnetem Qt-Ordnerdialog und aktivem OpenSSL-TLS-Backend bestand ohne Geräteabfrage. Eine Datenbank mit user_version 999 wurde mit Exit 1 abgewiesen und blieb bytegleich. Eine zweite Instanz wurde während einer laufenden ersten Instanz abgewiesen; die erste schloss regulär. Das lokale Signierwerkzeug besteht Syntax-/Hilfeprüfung und das Paketierungsskript bash -n. Screenshots der neuen Seiten wurden visuell geprüft; darunter ein korrigierter Profildialog und die sichtbar ausgewählte Standardaudioausgabe.

### CI-Kompatibilität der Untertitelgestaltung

Der erste Gesamt-CI-Lauf [41102240308](https://github.com/TimBooH/VELUNO/actions/runs/41102240308) baute unter Qt 6.4.2 vollständig. Sieben Testsuiten bestanden; der Player-Einstellungstest erkannte einen von Ubuntus älterem libmpv abgewiesenen sub-border-style-Befehl. Deshalb entstand kein Artefakt.

Die Implementierung fragt jetzt die tatsächliche option-info-Fähigkeit ab. Neuere Versionen verwenden die getrennte Stiloption; ältere wählen die Boxdarstellung über den Alpha-Wert von sub-back-color. Das entspricht dem offiziellen mpv-0.37-Quellcode in [sub/ass_mp.c](https://github.com/mpv-player/mpv/blob/v0.37.0/sub/ass_mp.c). Die Funktion bleibt vollständig erhalten; kein Test wurde übersprungen. Der Einstellungstest schaltet den Hintergrund jetzt ausdrücklich ein und aus und verlangt weiterhin einen fehlerfreien Player. Lokaler Neubau sowie Player-/Personal-/Backup-Tests bestanden nach der Korrektur.

### Abschließender Actions-Nachweis des Gesamtfunktionsausbaus

[Lauf 41104725560](https://github.com/TimBooH/VELUNO/actions/runs/41104725560) für `739e1f9307a2526b6551935ba0b0b50576df08ba` ist vollständig erfolgreich: Ubuntu 24.04 / Qt 6.4.2, Build, alle acht Testsuiten, begrenzter Programmstart, AppImage-Paketierung, isoliertes OpenSSL-TLS-Backend, synthetischer Medienimport (ein Film, null Fehler), geöffneter Ordnerdialog und Artefakt-Upload. Der zusätzliche Untertitelhintergrundtest besteht nun auch mit Ubuntus älterem libmpv.

[Aktuelles Artefakt 13001525493](https://github.com/TimBooH/VELUNO/actions/runs/41104725560/artifacts/13001525493) enthält AppImage, SHA-256-Datei und build-info.json. ZIP-Größe 118.597.866 Bytes, Ablauf laut GitHub am 13.11.2026 um 19:14:32 Uhr Europe/Berlin. ZIP-SHA-256: `eb1b53535748fd9fa5f73b5cd7f6fa931a4565a5ecd320fabcbd1f5fe03b6a78`.

Die noch gemeldeten statischen QML-Scannerhinweise betreffen Windows/macOS/iOS-Stile und das in C++ registrierte Veluno.Player; die echten Linux-Laufzeitprüfungen bestehen. Die bisherigen nicht blockierenden Paketierungs-/Actions-Hinweise gelten weiterhin. Echte TMDB-/Hosting-/NAS-/Audio-/GPU-/KWin-/HiDPI- und produktive Installationsabnahmen sind dadurch ausdrücklich nicht ersetzt. Der spätere Nutzerzugang und die dauerhafte signierte Updatequelle bleiben konfigurierte Voraussetzungen, keine vorgetäuschten Dienste.

Es wurde ausschließlich unstable verändert. main bleibt beim freigegebenen leeren Initial-Commit `864b7861add9787915cb714fc7bdf1260167b606`. Kein GitHub-Release wurde erstellt; dieser reine Nachweiscommit verwendet [skip ci].

## 30.10.2026 – Startseite und Poster überarbeitet

Auslöser war der Nutzerscreenshot: Die Bibliothek wirkte durch umrandete Verwaltungskarten, große Buchstaben-Platzhalter und technische Dateizähler wenig wie eine Streaming-Mediathek. Poster sind jetzt vollflächig im Format 2:3, mit dezenter Hover-Vergrößerung, fokussierbarer Wiedergabe und kompakten Titel-/Jahreszeilen. Ohne Bild bzw. bei defektem Cachebild bleibt der ausgeschriebene Titel als typografischer Ersatz sichtbar. Es werden keine Filmcover erfunden oder ohne Einwilligung geladen.

Weiterschauen nutzt deutlichere 16:9-Bilder mit unterem Verlauf und Cyan-Fortschritt. Beide Reihentypen haben Vor-/Zurück-Schaltflächen; Posterreihen auch Pfeiltasten. Die Kopfzeile ist kompakter und die gefüllte Startseite zeigt keine zusätzliche große Seitenüberschrift und Verwaltungsschaltflächen. Das Profilmenü bietet weiterhin Dateiöffnung; Ordnerverwaltung bleibt in Einstellungen/Bibliothek. Cover-Hinweis und anklickbare Scanstatuszeile führen unmittelbar dorthin. Sammlungsraster übernehmen die neue Kartenhöhe, damit Titelzeilen nicht überlappen.

UI-003 bleibt verbindlich: **kein großer Hero-Banner**, Reihenfolge Weiterschauen, Zuletzt hinzugefügt, Meine Filme, Meine Serien; leere Reihen ausgeblendet. Midnight Blue/Cyan und Light Mode bleiben erhalten. Eine anfängliche Ankündigung eines großen Titelbereichs wurde nach vollständiger erneuter PDF-Lektüre ausdrücklich korrigiert und nicht implementiert.

Neue QML-Prüfungen prüfen tatsächliche lokale Bildanzeige, Details-/Play-Unterscheidung, Reihenpfeile, Tastaturnavigation, beschädigten Bildpfad, Leerausblendung und Wege zu Einstellungen/Scanfehlern. Synthetische Poster entstehen ausschließlich im temporären Testverzeichnis, ohne externe Bildrechte oder Netzwerk. Dark-/Light-Screenshots wurden geprüft. Der neue Test musste zunächst eine noch laufende Scrollanimation abwarten, bevor er zum letzten Element sprang; das wurde im Test korrigiert. Manuelles Ziehen stoppt außerdem die programmatische Scrollanimation.

Die 62 Scanfehler und fehlenden Cover im Nutzerscreenshot sind **nicht als behoben bestätigt**: Es fehlen die konkreten Fehlerpfade/-meldungen und der Metadaten-Konfigurationszustand dieser Installation. Ohne TMDB-Zugang und Einwilligung gibt es weiterhin keinen Onlineabruf; eigene Bilder sind alternativ in den Titeldetails möglich. Bereits vorhandene QML-Dialog-implicitWidth-Warnungen bei Sprachumschaltung bestehen; die Bedienprüfungen bestehen. Zielsystem/NAS/GPU-Abnahme bleibt offen. Ausschließlich unstable, kein Release und keine Änderung an main.

Lokaler Abschlusslauf: Neubau erfolgreich, alle acht CTest-Suiten bestanden in 26,26 Sekunden. Der neue QML-Test prüft dieselben produktiven Komponenten; keine Tests übersprungen. AppImage/Ubuntu-Qt-6.4.2-Nachweis folgt über den Actions-Lauf dieses Commits.

Actions-Abschluss: [Lauf 41115879679](https://github.com/TimBooH/VELUNO/actions/runs/41115879679) für Quellcommit `e656d2da6e1c89b74ac52ed7298b6da1e6c1cbe2` ist erfolgreich. Build unter Ubuntu 24.04 / Qt 6.4.2, alle Tests, Programmstart, AppImage-Paketierung, isolierter Start mit OpenSSL, Import eines synthetischen Films ohne Fehler und geöffneter Ordnerdialog bestanden. [Artefakt 13004829132](https://github.com/TimBooH/VELUNO/actions/runs/41115879679/artifacts/13004829132): 118.600.799 Bytes, ZIP-SHA-256 `4efe636cd26e0b635bad0a5564873afa072d87dc2577684473a903890105a23c`, Ablauf am 13.11.2026 um 20:34:56 Uhr Europe/Berlin. Kein Stable-Release. main weiterhin unverändert auf `864b7861add9787915cb714fc7bdf1260167b606`.

## 30.10.2026 – Eigener Ein-/Aus-Schalter für TMDB

Auf ausdrücklichen Nutzerwunsch ersetzt ein eigener Switch „TMDB-Metadaten“ die bisherige Online-Auswahlbox unter Einstellungen → Bibliothek. Die TMDB-Einstellungen stehen direkt oben, mit sichtbarem Ein-/Aus-Status, verständlichem Hinweis auf gesendete Titel, verdecktem Tokenfeld und manueller Aktualisierung. Die vorhandene TMDB-Anbindung für Beschreibungen, Genres, Bewertungen, Poster und Hintergründe bleibt vollständig erhalten. Kein API-Zugang wird eingebettet oder erfunden.

MetadataController.enabled verwendet dieselbe persistierte Online-Entscheidung wie der Wizard. Token speichern ist unabhängig und erteilt keine Zustimmung. Aus bricht aktive Requests ab und leert die Warteschlange; verspätete Antworten dürfen keine Metadaten mehr übernehmen. Gespeicherte Offlineinformationen/Bilder und Zugang bleiben erhalten. Ein stößt die automatische Ergänzung erneut an. Eine fehlgeschlagene Speicherung darf den Abruf nicht aktivieren. Keine Schemaänderung erforderlich.

Neue Tests nutzen einen lokalen API-Server mit absichtlich zurückgehaltener Antwort: Ausschalten trennt die aktive Verbindung, verhindert weitere Anfragen, erhält den Cache und überlebt Controller-Neustart. Wieder einschalten ruft automatisch ab; Wizard und Schalter bleiben synchron. Ein schreibgeschützter Datenbankfall bleibt ausgeschaltet. Die Oberfläche prüft echten Mausklick, Token/Freigabe-Trennung, Ein-/Aus-Zustand und deaktivierte Aktualisierung. Ein erster UI-Test erreichte den anfangs unter den Medienordnern liegenden Switch nicht; TMDB wurde deshalb an den Anfang des Bibliotheksbereichs gestellt und der Test wartet auf fertiges Layout. QML-Tests nutzen jetzt ausdrücklich eine lokale API-Adresse, damit Testzugänge niemals an den echten Anbieter gesendet werden.

Neubau und gezielte Backend-/Oberflächenprüfungen bestanden nach dieser Korrektur. Dark-/Light-Darstellung wird als Screenshot geprüft. Die vorhandenen Dialog-implicitWidth-Warnungen bei Sprachumschaltung bleiben bekannt; der absichtlich fehlende Bildpfad des Postertests meldet weiterhin die erwartete Bildwarnung. Ein echter TMDB-Zugang liegt nicht vor; produktive Anbieter-/Bildrechte-Abnahme bleibt wie zuvor offen. Entwicklung ausschließlich auf unstable, kein Stable-Release und keine Änderung an main.

Lokaler Abschluss: alle acht CTest-Suiten bestanden in 30,28 Sekunden, einschließlich erweitertem metadata_update und echtem UI-Switch-Test. Beide Theme-Screenshots visuell geprüft. AppImage-Nachweis folgt im Actions-Lauf dieses Quellcommits.

Actions-Abschluss: [Lauf 41119267290](https://github.com/TimBooH/VELUNO/actions/runs/41119267290) für `a0d27701fc3a049ecb1cb59b25604279306fe6d2` ist erfolgreich. Alle acht Testsuiten bestanden unter Ubuntu/Qt 6.4.2 in 35,74 Sekunden. Gepacktes AppImage startet mit OpenSSL-TLS-Backend, importiert einen synthetischen Film ohne Scanfehler und öffnet den Ordnerdialog. [Artefakt 13006819354](https://github.com/TimBooH/VELUNO/actions/runs/41119267290/artifacts/13006819354): ZIP 118.607.317 Bytes, SHA-256 `24d582fc2d5971772f5007908c1e808b9c5adc4c7c34c1aaabd844a3d3f5f29a`, GitHub-Ablauf 13.11.2026 um 21:00:13 Uhr Europe/Berlin. Kein Live-TMDB-Zugang getestet. main weiterhin beim leeren Initial-Commit `864b7861add9787915cb714fc7bdf1260167b606`, kein Release.

## 30.10.2026 – Film- und Seriendetails als Streaming-Titelansicht

Der Nutzer präzisierte anhand des Screenshots, dass die Ansicht **nach Klick auf einen Film** wie eine Streaming-Titelansicht wirken soll. Der bisherige technische Dateidialog wurde durch TitleDetails.qml ersetzt: großer vollflächiger lokaler Hintergrund (Poster als Ersatz), Verlauf, großer Titel und primäre Wiedergabe; darunter Jahr, Laufzeit, vorhandene Bewertung/Genres/Beschreibung sowie Favorit, Gesehen und Sammlungen. Ein ständig erreichbares X schließt die Ansicht, ebenso Escape. Inhalt und Folgen scrollen in einer gemeinsamen Liste. Ohne Onlinebilder bleiben Titel und Bedienung über einem neutralen Hintergrund verfügbar. Es werden keine fremden Bilder oder Vorschauvideos eingebettet.

Dateipfade, manuelle Dateizuordnung und Filmversionen sind unter „Dateien und Versionen“ aufklappbar. Die höchste Auflösung bleibt Standard; vorhandene Versionen können gezielt gespielt werden. Bei Serien sind Staffelauswahl und Episoden standardmäßig zugänglich; der primäre Knopf führt zur Folgenliste. Informationen-/Bildbearbeitung und Sammlungszuordnung bleiben in den vorhandenen Unterdialogen. Der TMDB-Schalter gilt auch für Onlineaktionen im Informationseditor; lokale Bearbeitung/Bildwahl bleibt möglich. Keine Funktion wurde entfernt, keine Datenbankmigration und keine Änderung an Originalmedien.

LibraryController stellt die bereits geladenen Detaildateien als schreibgeschützte QVariantList bereit. Auswahländerungen werden erst nach Aktualisierung dieser Dateiliste signalisiert; laufende Scans/Metadatenaktualisierungen erneuern dadurch auch offene Details konsistent. Kein zusätzlicher Dateisystemzugriff im UI. Beim Schließen während einer SMB-Verbindungsauflösung wird die ausstehende Wiedergabe verworfen.

Neue echte QML-Bedienprüfungen decken primäre Filmwiedergabe, lokal gewähltes Hintergrundbild, Beschreibung, Favoriten, verborgenes/aufgeklapptes Dateimodell, Dateizuordnung/Abbruch, Staffelauswahl, Episodenwiedergabe, Informationseditor, Sammlungen, Mindestfenstergröße und Escape/X ab. Synthetische Serien über zwei Staffeln und selbst gezeichnetes Testbild liegen nur im temporären Testverzeichnis. Die ersten Layout-/Kontrastprüfungen fanden notwendiges Warten auf das tatsächliche Fenster-Resize sowie eine unzuverlässig ausgelesene Akzent-Textfarbe: die Theme-Eigenschaft heißt nun eindeutig accentText statt onAccent. Der Test verlangt im hellen Design tatsächlich weiße Beschriftung des dunklen Wiedergabeknopfs. Ein heller Bildverlauf wurde ebenfalls für lesbare weiße Titel korrigiert. Kein Test wurde übersprungen.

Die Startseite bleibt unverändert ohne großen Hero-Banner (UI-003); dieser große Bildbereich gehört ausschließlich zur nun ausdrücklich gewünschten Titeldetailansicht. Beide Themes und DE/EN bleiben unterstützt. Reale TMDB-Zugänge, NAS, KDE/HiDPI und Hardwareabnahme bleiben weiterhin offen; bestehende Dialog-implicitWidth-Warnungen bei Sprachwechsel und die absichtliche Bildfehlerwarnung des Fallbacktests werden nicht als behoben ausgegeben. Entwicklung ausschließlich unstable, kein Release und keine Änderung an main.

Lokaler Abschlusslauf nach den Kontrast-/Resize-Korrekturen: Neubau erfolgreich, alle acht CTest-Suiten bestanden in 32,29 Sekunden, QML darin 10,31 Sekunden. Dark-/Light-Titeldetails und Serienansicht im 980×600-Fenster visuell geprüft. Ubuntu-/AppImage-Nachweis folgt über Actions.

Abschließender Übersetzungscheck ergänzt auch Episoden und Schließen auf Englisch. Der erste Actions-Lauf für die Detailansicht wartete zu diesem Zeitpunkt noch auf einen Runner; der folgende Quellstand enthält diese beiden Beschriftungen ebenfalls.

Actions-Abschluss für den endgültigen Quellstand `939dfc263a54103a41ffa149fc250caff70b1593`: [Lauf 41122070208](https://github.com/TimBooH/VELUNO/actions/runs/41122070208) ist erfolgreich. Alle acht Testsuiten bestanden unter Ubuntu/Qt 6.4.2 in 36,61 Sekunden, QML in 9,77 Sekunden. Gepacktes AppImage startet mit OpenSSL-TLS, importiert einen synthetischen Film ohne Fehler und öffnet den Ordnerdialog. [Artefakt 13009482120](https://github.com/TimBooH/VELUNO/actions/runs/41122070208/artifacts/13009482120): ZIP 118.615.156 Bytes, SHA-256 `4128df7730d008edd915d77c671d0d391a92a2c4b5b5f8ee58c86cfcadd951dc`, Ablauf bei GitHub am 13.11.2026 um 21:24:14 Uhr Europe/Berlin. Die vorherige Runner-Wartezeit ist beendet. Kein Release, main bleibt unverändert beim freigegebenen leeren Initial-Commit `864b7861add9787915cb714fc7bdf1260167b606`.

## 31.10.2026 – Fünf angeforderte Verbesserungsbereiche

Der Nutzer hat die fünf zuvor empfohlenen Bereiche ausdrücklich zur Umsetzung beauftragt: Erkennung/Scanfehler, TMDB-Abnahmeabläufe, Fortschritt/Titeldetails, Bedienung und Wiedergabeprüfung. Implementiert wurden zusätzliche konservative Serienmuster und bessere Jahres-/Staffelabgrenzung, längere SMB-Probezeit, konkrete Fehlerhinweise und kopierbare Fehlerpfade; TMDB-Zugangsprüfung/Statistik/Fehlbild-Wiederholung samt gemeinsam begrenzter Bild-/API-Warteschlange; profilbezogenes Fortsetzen für Filme und die letzte begonnene Serienfolge, sichtbare Fortschritts-/Gesehen- und Qualitätsangaben; umbrechende Metadaten-/Playeraktionen, Hauptaktionsfokus und Pos1/Ende; tatsächlich beobachtete Player-Decoder-/Audio-/Video-Diagnosen. Originaldateien bleiben unverändert, keine neue Migration, keine Änderung an main und kein Stable-Release.

Neue bzw. erweiterte Tests prüfen zusätzliche Dateinamensmuster einschließlich unklarer Staffelpfade, Zahlenfilmen und [REC]; echtes Wiederaufnehmen aus persönlichem Fortschritt; primäre Fortsetzen-Aktion/Serienfolge; Player-Diagnose und weiterhin Audio/Untertitel/Vollbild; authentifizierte TMDB-Konfiguration, abgewiesenen/korrigierten Zugang, tatsächlichen lokalen PNG-Download/-Dekodierung und Offline-Erhalt. Die Produktionsadressen bleiben HTTPS/TMDB; lokale Bildquelle ist ausschließlich ein injizierbarer Konstruktor-Testparameter, keine Benutzereinstellung. Ohne Token/Einwilligung weiterhin keine Abfragen. Geheimnisse erscheinen nicht in URLs oder Diagnoseanzeigen.

Korrigierte Entwicklungsfehler: Der pure Parser darf für synthetische/nicht gemountete Pfade kein QDir.cdUp mit physischer Existenzprüfung benötigen; er steigt jetzt lexikalisch bis zur Quellgrenze auf. Home/End werden über das allgemeine Keys-Ereignis behandelt, da dafür keine eigenen Qt-Keys-Signale verfügbar sind. Der UI-Test wartet vor Klick auf den Fortsetzen-Knopf auf dessen sichtbare Scrollposition. Keine Tests wurden übersprungen. Zahlen mit Jahresbedeutung bleiben Dateinamen-Heuristik und können manuelle Korrektur benötigen.

Die konkreten 62 Scanfehler und 76 nicht zugeordneten Dateien des Nutzers sowie echtes NAS/TMDB-Konto/Audio/GPU/HDR/KWin/HiDPI sind hier nicht verfügbar. Sie sind **nicht als erfolgreich abgenommen** ausgewiesen. TARGET_TESTS.md enthält die konkreten Nachweise für das Zielgerät. Vorhandene QML-Dialog-implicitWidth-Warnungen beim Sprachwechsel und die beabsichtigte Warnung des kaputten Posterpfadtests bleiben bekannt.

Die visuelle Prüfung korrigierte außerdem die Statistik: provider_id 0 bezeichnet lokale/manuelle Informationen und darf nicht als TMDB-Zuordnung zählen. Der Test legt deshalb ausdrücklich einen lokal bearbeiteten Titel ohne Provider an. Fortschrittsbalken erhalten eine explizite Cyan-Darstellung, deaktivierte Metadatenaktionen sind visuell zurückgenommen, und der neue Diagnose-Schließenknopf ist lokalisiert. Eine zusätzliche MKV-Prüfung mit englischer Standardspur und deutscher Alternativspur prüft tatsächliche deutsche Vorauswahl, manuellen Wechsel auf Englisch und erneute deutsche Auswahl beim nächsten Öffnen. aid wird beim Dateistart auf auto zurückgesetzt; individuelle Spurwahl gilt weiterhin für die laufende Datei.

Lokaler kompletter Lauf vor diesem zusätzlichen Audiospurtest: alle acht Suiten bestanden in 39,15 Sekunden. Anschließend Metadaten-/QML-Prüfungen nach Statistik-/Darstellungskorrektur bestanden sowie Home/End ohne direkten programmatischen Sprung geprüft. Finaler Audiospur-/CI-Nachweis folgt.

Finaler lokaler Gesamtlauf nach der Audiospurkorrektur: alle acht Suiten bestanden in 39,32 Sekunden, darunter Player 2,34 s, Metadaten/Updates 13,02 s und QML 10,97 s. Der zusätzliche Sprachspurtest besteht mit zwei echten AAC-Spuren und unveränderter Medienprüfsumme. Build, neue DE/EN-Beschriftungen und Screenshots der Fortsetzen-/TMDB-/Diagnoseansichten wurden geprüft. Der folgende Actions-Lauf prüft die Auslieferung unter Ubuntu/Qt 6.4.2.

Der erste Actions-Lauf [41153004669](https://github.com/TimBooH/VELUNO/actions/runs/41153004669) für b94a946 bestand bereits Build, alle Tests, Start und AppImage-Prüfung. Der abschließende Erkennungsvergleich ergänzte noch eine gezielte Korrektur: „Extras“ und „Specials“ sind zulässige Titel und werden nicht pauschal als Staffelordner behandelt. Zwei neue Regressionstests bestehen; lokaler Bibliothekslauf 4,40 Sekunden. Der endgültige AppImage-Lauf enthält auch diese Korrektur.

### Reproduzierbarer Qt-6.4-Absturz bei Fensterverkleinerung

Der nachfolgende Actions-Lauf [41153497413](https://github.com/TimBooH/VELUNO/actions/runs/41153497413) für 87a415f bestand sieben Suiten, stürzte aber im QML-Test nach den Filmdetails ab; deshalb wurde kein Artefakt erzeugt. Ein erfolgreicher vorheriger Lauf reichte somit nicht als Zuverlässigkeitsnachweis.

Eine isolierte Ubuntu-24.04-/Qt-6.4.2-Umgebung wurde außerhalb des Repositorys aufgebaut. GDB reproduzierte den Fehler nach wenigen Wiederholungen: qmlAttachedPropertiesObject/QMetaObject::inherits aus QQuickGridLayoutBase::rearrange während einer Fenstergrößenänderung. Zusätzliche temporäre Objektketten-Diagnostik identifizierte die ColumnLayout im Spuren-Popup des Players. Dessen Repeater entfernt und erzeugt Audiospur-/Untertitelknöpfe bei Dateiwechseldaten; der Qt-6.4-Layoutcache konnte beim nachfolgenden Resize einen ungültigen Eintrag verwenden, auch wenn das Popup geschlossen war. Der Fehler war im Rückblick bereits im früheren Lauf 41121876710 mit derselben Abbruchstelle sichtbar.

Das Spurenmenü verwendet deshalb jetzt eine scrollbare Column statt des dynamischen Layoutcaches. Auch die beim Sprachwechsel erneuerten Diagnosezeilen verwenden einen Positionierer. Spurenwahl, externe Untertitel, ausgewählte Spur und Schließen bleiben erreichbar; das Menü passt sich kleinen Fenstern an und hält das Playeroverlay während der Bedienung offen. Die Regression öffnet das Menü, lädt die Datei erneut, verkleinert/vergrößert das Fenster mehrfach und bedient danach weiterhin Player/Zeitleiste. Eine weitere Prüfung kontrolliert die Griffposition beim Drücken der Zeitleiste; zwölf Dateineuladungen mit vorübergehend blockierter UI-Ereignisschleife bestätigen weiterhin Dauer und Audiospuren. Für geometrieabhängige Eingaben wartet der Test auf Render-Ereignisse. Der zwischenzeitlich beobachtete einzelne Sprung-Assertionfehler wird nicht als weiterer Absturz ausgegeben.

Temporäre Debug-Namen, Konsolenmarker und der GDB-Helfer wurden vollständig entfernt. Docker/GDB gehören ausschließlich zur Entwicklungsdiagnose, nicht zum Programm oder AppImage. Keine Tests wurden übersprungen und die Mindest-Qt-Version wurde nicht angehoben.

Bereinigter Stand: alle acht Suiten unter Ubuntu/Qt 6.4.2 bestanden in 45,42 Sekunden, einschließlich nativem Wayland-Bildtest; Player/QML unter Qt 6.8.2 zusätzlich in 21,45 Sekunden. Anschließend folgen weitere unveränderte QML-Wiederholungen und der finale Actions-Auslieferungsnachweis.

Zehn abschließende, instrumentierungsfreie Qt-6.4-QML-Durchläufe mit erweitertem Resize-/Spurenmenü-/Zeitleistentest bestanden hintereinander (je 16 Fälle, null Fehler, null übersprungen; ungefähr 15 Sekunden pro Durchlauf). Damit wurde der zuvor sporadische Absturz gezielt mit derselben Qt-Version nachgeprüft.

Finale Auslieferung: [Actions-Lauf 41157032462](https://github.com/TimBooH/VELUNO/actions/runs/41157032462) für `d40ac4c7343819ee0e20973ec032963a91de6e73` vollständig erfolgreich. Unter Ubuntu 24.04 / Qt 6.4.2 bestanden alle acht Suiten in 46,58 Sekunden, QML darin 15,45 Sekunden. Gepacktes AppImage startet mit OpenSSL-TLS, importiert einen synthetischen Film ohne Fehler und öffnet den Ordnerdialog. [Artefakt 13026076322](https://github.com/TimBooH/VELUNO/actions/runs/41157032462/artifacts/13026076322): ZIP 118.752.085 Bytes, SHA-256 `e1e7752873ec614418663249ba130711fe76c76492acb7ac209629c4e8c91844`, GitHub-Ablauf am 14.11.2026 um 02:51:50 Uhr Europe/Berlin. Der zuvor reproduzierte Resize-Absturz ist mit derselben Qt-Version und erweiterten Tests nachgeprüft. main bleibt bei `864b7861add9787915cb714fc7bdf1260167b606`; kein Stable-Release. Die Zielgerät-/Nutzerbibliotheks-/TMDB-Konto-Abnahmen aus TARGET_TESTS.md bleiben offen.

## 31.10.2026 – Vorbereitung der ausdrücklich freigegebenen Stable-Version

Nutzerauftrag: „Mach die Version von unstable jetzt öffentlich in Main stable“. Ausschließlich auf unstable werden Buildkennung, AppImage-Dateiname und der auf v0.1.0 begrenzte Release-Workflow vorbereitet. main erhält anschließend den getesteten Commit per Fast-forward; der Tag startet erneut Tests und Paketprüfung vor Veröffentlichung. Keine Datenbankmigration und keine Änderung der Medienfunktionen. Freigabe und bekannte offene Abnahmen stehen unter docs/releases/. Die älteren Aussagen „kein Stable/main unverändert“ dokumentieren den damaligen Verlauf und sind durch diese konkrete Freigabe überholt.

Releasevorprüfung: Ubuntu 24.04 / Qt 6.4.2 mit `VELUNO_STABLE_BUILD=ON` erfolgreich neu gebaut; alle acht Testsuiten bestanden in 45,73 Sekunden. Buildinformationen melden Version 0.1.0, Kanal stable und Schema 4; separater Starttest mit OpenSSL-TLS erfolgreich. Die Über-Ansicht verwendet jetzt dieselbe Buildversionskennung statt einer festen dev-Zeichenfolge; deutsche und englische Texte sind angepasst. YAML und sämtliche Shell-Blöcke des neuen Workflows sowie das Paketskript sind syntaktisch geprüft. Der GitHub-Paket-/Veröffentlichungsnachweis folgt erst nach erfolgreichem Lauf.

## 31.10.2026 – Stable v0.1.0 veröffentlicht

Der geprüfte Commit `3dcbe9666aff47b0c83af331fe50a64b3a963945` wurde ohne Force-Push von unstable nach main übernommen; der annotierte Tag v0.1.0 zeigt auf denselben Commit. Entwicklung und die abschließende Dokumentation bleiben auf unstable.

Vorprüfung [41158692062](https://github.com/TimBooH/VELUNO/actions/runs/41158692062): alle acht Suiten in 47,09 Sekunden sowie AppImage-Pakettests erfolgreich. Abschließender [Stable-Lauf 41159123633](https://github.com/TimBooH/VELUNO/actions/runs/41159123633): Build und Publish erfolgreich, alle acht Suiten in 47,51 Sekunden, OpenSSL-TLS/Start, synthetischer Filmimport (ein Film, null Fehler) und Ordnerdialog im gepackten AppImage bestanden. Hochgeladene Assets wurden im Entwurf auf Vollständigkeit/Dateigröße und vor dem Upload auf Prüfsumme geprüft.

[Öffentlicher Release v0.1.0](https://github.com/TimBooH/VELUNO/releases/tag/v0.1.0), veröffentlicht am 31.10.2026 um 03:19:43 Uhr Europe/Berlin (02:19:43 UTC), kein Entwurf und kein Prerelease. Dauerhafter [AppImage-Download](https://github.com/TimBooH/VELUNO/releases/download/v0.1.0/VELUNO-0.1.0-x86_64.AppImage): 119.486.968 Bytes, SHA-256 `4e5e38b3ad055dd2031ae442c47b2bd482cc4c3cab6be5254ddd8134422bc03a`. Öffentliche AppImage-, Prüfsummen- und Buildinfo-URLs anschließend tatsächlich abgerufen; SHA-256-Prüfung erfolgreich. Buildinfo: Version 0.1.0, Kanal stable, Schema 4, Buildzeit 2026-10-31T02:15:47Z. Diese Downloads sind Release-Assets und laufen nicht wie Actions-Artefakte nach 14 Tagen ab.

Die in TARGET_TESTS.md und O-001 bis O-010 genannten offenen Zielgerät-/Anbieter-/Nutzerbibliotheksnachweise bleiben offen und sind in den öffentlichen Releasehinweisen genannt. Keine Freigabe zukünftiger Stable-Versionen; main und Tag werden durch diesen Dokumentationsabschluss nicht verändert.

## 31.10.2026 – Streaming-Design auf unstable

Nutzerwunsch: „Das Design sieht nicht aus wie Netflix. Können wir das noch ändern?“ Nach vollständiger Prüfung des Pflichtenhefts wurde die Bannerfrage ausdrücklich geklärt: „Startseite ohne Banner; großes Motiv in den Filmdetails“. UI-003 und die vorhandene Reihenfolge bleiben erhalten. Keine Änderung an main, Tag oder öffentlichem Stable-Release.

Dunklere Midnight-Blue-Flächen, größere Wortmarke, dichtere Posterreihen mit einer Textzeile statt zusätzlicher Jahres-/Typzeile und dezenter typografischer Darstellung ohne Bilder. Die Details verwenden ein großes vollflächiges Motiv, weichen Verlauf, größere Titel, weiße Abspielen-/Fortsetzen-Aktion, runde Favoriten-/Gesehen-/Sammlungsaktionen mit ausgeschriebenen zugänglichen Namen und Tooltips sowie ein Qualitätslabel. Größe und bildloser Zustand sind an kleine Fenster angepasst. Light Mode behält einen hellen Informationsbereich und kontrastreiche Aktionen über dem dunklen Motiv. Keine neue Metadatenabfrage, Migration oder Änderung an Player/Medienlogik. Geprüfte Screenshots und Gestaltungsentscheidungen: DESIGN.md.

Erster Prüflauf: sieben Suiten bestanden, QML meldete zwei Fehler: erwarteter alter Hintergrundfarbwert und inkonsistente Seitenschrittweite bei der Umstellung der Pfeiltasten auf sofortige Navigation. Farbwert aktualisiert und gleiche Seitenschrittweite für Maus/Tastatur wiederhergestellt; keine Prüfung entfernt oder übersprungen. Vollständiger Abschlusslauf unter Ubuntu 24.04 / Qt 6.4.2: alle acht Suiten bestanden in 47,04 Sekunden, inklusive nativem Wayland-Render-Test. Nach abschließender Größen-/Kontrastanpassung QML erneut in 16,22 Sekunden bestanden, Start mit OpenSSL-TLS erfolgreich. Die vorhandenen Dialog-implicitWidth-Warnungen beim Sprachwechsel bleiben dokumentiert; echte KWin-/HiDPI-/GPU-Abnahmen sind weiterhin offen. Der Landschafts-Testhintergrund wird ausschließlich im Test erzeugt, ohne Fremdposter oder ausgelieferte Beispielbibliothek. Ein GitHub-AppImage-Nachweis folgt über den Unstable-Workflow.
