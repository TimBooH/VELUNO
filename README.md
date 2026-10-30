# VELUNO · 0.1.0-dev

Native Film- und Serienmediathek für Linux. VELUNO ist ein vorläufiger Arbeitsname.
Diese Entwicklungsstufe enthält die technische Grundlage, lokale Videowiedergabe mit libmpv und eine lokale Film-/Serienbibliothek mit Medienordnern und Hintergrundscan.

## Linux-Build

Benötigt: C++20-Compiler, CMake ≥ 3.22, Qt ≥ 6.4 (Core, Gui, Quick, QuickControls2, Sql, DBus, Test, QuickTest), SQLite-Qt-Treiber, libmpv-Entwicklungspaket und pkg-config. FFmpeg/ffprobe wird zur Medienanalyse benötigt und mit dem AppImage ausgeliefert.

CachyOS/Arch:

```sh
sudo pacman -S --needed base-devel cmake ninja qt6-base qt6-declarative qt6-wayland mpv ffmpeg weston
```

Debian 13 / Ubuntu 24.04:

```sh
sudo apt install build-essential cmake ninja-build pkg-config qt6-base-dev qt6-declarative-dev libqt6sql6-sqlite libmpv-dev qt6-wayland qml6-module-qtqml qml6-module-qtqml-models qml6-module-qtquick-shapes qml6-module-qt-labs-folderlistmodel qml6-module-qtquick qml6-module-qtquick-controls qml6-module-qtquick-dialogs qml6-module-qtquick-layouts qml6-module-qtquick-window qml6-module-qtqml-workerscript qml6-module-qtquick-templates qml6-module-qttest ffmpeg weston
```

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/veluno
```

Für einen nativen Wayland-Start in einer laufenden Sitzung:

```sh
QT_QPA_PLATFORM=wayland ./build/veluno
```

Begrenzter Starttest ohne Desktop:

```sh
QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software ./build/veluno --smoke-test
```

Das Design wird über Profilmenü → Einstellungen → Allgemein gewählt und in SQLite gespeichert. Die Datenbank liegt unter `QStandardPaths::AppDataLocation` (üblicherweise `~/.local/share/VELUNO/VELUNO/veluno.sqlite3`). Ein Initialisierungsfehler beendet den Start mit Diagnose auf stderr; eine defekte oder neuere Datenbank wird nicht gelöscht oder ersetzt.

## Medien importieren

Beim ersten Start den Einrichtungsassistenten durchlaufen und Film- oder Serienordner auswählen. Später sind Ordner über **Medienordner** oder **Profil → Einstellungen → Bibliothek** erreichbar. Scans laufen im Hintergrund und lassen sich pausieren. Unklare Dateinamen können unter **Nicht zugeordnet** korrigiert werden. Filmversionen und Serienepisoden sind über die Kacheln erreichbar. Die Suche oben durchsucht lokale Film-/Serientitel.

Online-Metadaten und Cover werden noch nicht abgerufen. Die entsprechende Entscheidung wird ausdrücklich gespeichert. Details, Dateinamensmuster und Grenzen: [Lokale Bibliothek](docs/LIBRARY.md). Migration 2 aktualisiert bestehende Datenbanken; ältere Builds mit Schema 1 können sie anschließend nicht öffnen. Eine automatische Sicherung/Wiederherstellung folgt später.

## Eine Datei abspielen

Auf der Startseite **Datei abspielen** wählen oder `Strg+O` drücken. Die Wiedergabe erfolgt im VELUNO-Fenster. Weitere Dateien sind auch über das Profilmenü erreichbar. **Zurück** beendet die Wiedergabe.

Tasten: Leertaste Pause/Fortsetzen, Links/Rechts ±10 Sekunden, Hoch/Runter Lautstärke, F Vollbild, Esc Vollbild verlassen, M stumm, S Untertitelspur, A Audiospur. Geschwindigkeit 0,5×–2× und Zeitleiste sind im Overlay erreichbar. Das Overlay verschwindet nach zwei Sekunden Inaktivität und bleibt bei Pause sichtbar. Die letzte Lautstärke wird beim regulären Beenden gespeichert.

Diese erste Stufe verwendet die libmpv-Software-Render-API auf einem eigenen Worker-Thread. Die UI erhält nur das jeweils neueste Bild. Eine GPU-Renderintegration und eine Leistungsabnahme für 4K/HDR sind noch nicht abgeschlossen. Fortsetzen/Gesehen-Status, einstellbare Tastenkürzel, Gerätewahl und Serienwechsel folgen später. Alle Anforderungen bleiben in der Anforderungsübersicht erhalten.

Die Tests benötigen FFmpeg für synthetische Testvideos. Wenn Weston installiert ist, führt CTest zusätzlich einen nativen Wayland-Bildtest mit Software-Compositor aus. Ohne Weston bleibt der Grafiktest manuell über `build/player_render_tests -platform wayland` in einer laufenden Sitzung ausführbar; er wird nicht als bestanden gewertet, wenn er nicht ausgeführt wurde.

## Entwicklungsregeln

- Ein Repository, Entwicklung ausschließlich auf `unstable`.
- `main` enthält vorerst ausschließlich den ausdrücklich erlaubten leeren Initial-Commit.
- Kein Release-/Publish-Workflow. Unstable-Pakete ausschließlich als Actions-Artefakte nach Tests.
- Der technische Entwicklungsstand wurde nach ausdrücklicher Freigabe auf `unstable` übertragen. Stable-Releases benötigen weiterhin eine gesonderte Freigabe.

[Architektur](docs/ARCHITECTURE.md) · [Entwicklungsprotokoll](docs/DEVELOPMENT.md) · [Anforderungsstatus](docs/REQUIREMENTS.md) · [Pflichtenheft](docs/VELUNO_v0.1.0_Gesamt-Pflichtenheft.pdf)
