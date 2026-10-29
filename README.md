# VELUNO · 0.1.0-dev

Native Film- und Serienmediathek für Linux. VELUNO ist ein vorläufiger Arbeitsname.
Diese Entwicklungsstufe enthält ausschließlich die technische Grundlage, keine Medienverwaltung oder Wiedergabe.

## Linux-Build

Benötigt: C++20-Compiler, CMake ≥ 3.22, Qt ≥ 6.4 (Core, Gui, Quick, QuickControls2, Sql, Test, QuickTest), SQLite-Qt-Treiber, libmpv-Entwicklungspaket und pkg-config.

CachyOS/Arch:

```sh
sudo pacman -S --needed base-devel cmake ninja qt6-base qt6-declarative qt6-wayland mpv
```

Debian 13 / Ubuntu 24.04:

```sh
sudo apt install build-essential cmake ninja-build pkg-config qt6-base-dev qt6-declarative-dev libqt6sql6-sqlite libmpv-dev qt6-wayland qml6-module-qtquick qml6-module-qtquick-controls qml6-module-qtquick-layouts qml6-module-qtquick-window qml6-module-qtqml-workerscript qml6-module-qtquick-templates qml6-module-qttest
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

## Entwicklungsregeln

- Ein Repository, Entwicklung ausschließlich auf `unstable`.
- `main` enthält vorerst ausschließlich den ausdrücklich erlaubten leeren Initial-Commit.
- Kein Release-/Publish-Workflow. Unstable-Pakete ausschließlich als Actions-Artefakte nach Tests.
- Lokaler Quellcode wird erst nach Freigabe in das öffentliche Repository übertragen.

[Architektur](docs/ARCHITECTURE.md) · [Entwicklungsprotokoll](docs/DEVELOPMENT.md) · [Anforderungsstatus](docs/REQUIREMENTS.md) · [Pflichtenheft](docs/VELUNO_v0.1.0_Gesamt-Pflichtenheft.pdf)
