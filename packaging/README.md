# AppImage / Unstable

`appimage.sh BUILDVERZEICHNIS AUSGABEVERZEICHNIS` paketiert einen vorher getesteten Build und prüft das Paket im Offscreen-Modus mit neuem Datenverzeichnis. Das Ausgabeverzeichnis muss frisch sein, damit keine Reste älterer Pakete übernommen werden. Das Skript lädt nichts hoch. `ffprobe` wird als eigenes Programm mit seinen Bibliotheksabhängigkeiten paketiert. Die isolierte Prüfung importiert ein synthetisches Video mit dem im Paket enthaltenen Scanner und erwartet einen Filmtitel ohne Fehler.

linuxdeploy und das Qt-Plugin werden per SHA-256 auf den am 29.10.2026 geprüften Inhalt festgelegt. Die verwendeten `continuous`-URLs können sich ändern; dann muss die Prüfsumme nach erneuter Prüfung bewusst aktualisiert werden. Eine abweichende Datei bricht die Paketierung ab.

Wayland-, Offscreen- und SQLite-Plugins werden ausdrücklich mitgenommen. Die Paketprüfung entfernt die Qt-/Bibliothekspfade der Build-Umgebung aus dem Kindprozess, damit fehlende Plugins nicht durch die installierten Entwicklungspakete verdeckt werden. FUSE-unabhängige Prüfung erfolgt mit `APPIMAGE_EXTRACT_AND_RUN=1`. Reguläres FUSE-Mounting und Auslieferung auf dem Zielsystem sind noch separat abzunehmen.

Der Workflow `.github/workflows/unstable.yml` läuft ausschließlich für `unstable`, baut unter Ubuntu 24.04, testet und lädt erst nach erfolgreicher Paketprüfung ein kurzlebiges Artefakt hoch. Es gibt keine GitHub-Release-Aktion, keine Schreibberechtigung für Repository-Inhalte und keinen automatischen Stable-Publish. Auf `main` läuft dieser Workflow auch bei manuellem Start nicht.

Der Workflow wurde nach Upload-Freigabe erfolgreich auf GitHub unter Ubuntu 24.04 / Qt 6.4.2 ausgeführt: [Lauf 40949745517](https://github.com/TimBooH/VELUNO/actions/runs/40949745517), einschließlich isoliertem Paketstart und [Artefakt-Upload](https://github.com/TimBooH/VELUNO/actions/runs/40949745517/artifacts/12930812787). Das bestätigt noch keine CachyOS-Abnahme. Mindestglibc, Zielhardware, FUSE, Codec-/GPU-Abhängigkeiten und Drittanbieter-Lizenzen bleiben Teil der späteren Distributionsabnahme (O-004, O-010).

Aktueller Bibliotheksstand: [Lauf 41069263260](https://github.com/TimBooH/VELUNO/actions/runs/41069263260) und [Artefakt 12984037864](https://github.com/TimBooH/VELUNO/actions/runs/41069263260/artifacts/12984037864). Alle fünf Testsuiten sowie isolierter AppImage-Start mit Import und paketiertem ffprobe bestanden unter Qt 6.4.2. Das Artefakt gehört zum Quellcommit `021b99d37367c2220fd8c21601eb9a5ac02c69cc`; spätere reine Dokumentationscommits verändern es nicht.

Zusätzlich zum Medienimport wird der Ordnerdialog im isolierten AppImage tatsächlich geöffnet und geschlossen (`--smoke-test --smoke-dialog`). `Qt.labs.folderlistmodel` wird dafür ausdrücklich eingebunden; QtQml.Models und QtQuick.Shapes gehören ebenfalls zu den CI-Laufzeitabhängigkeiten.
