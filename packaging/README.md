# AppImage / Unstable

`appimage.sh BUILDVERZEICHNIS AUSGABEVERZEICHNIS` paketiert einen vorher getesteten Build und prüft das Paket im Offscreen-Modus mit neuem Datenverzeichnis. Das Ausgabeverzeichnis muss frisch sein, damit keine Reste älterer Pakete übernommen werden. Das Skript lädt nichts hoch.

linuxdeploy und das Qt-Plugin werden per SHA-256 auf den am 29.10.2026 geprüften Inhalt festgelegt. Die verwendeten `continuous`-URLs können sich ändern; dann muss die Prüfsumme nach erneuter Prüfung bewusst aktualisiert werden. Eine abweichende Datei bricht die Paketierung ab.

Wayland-, Offscreen- und SQLite-Plugins werden ausdrücklich mitgenommen. Die Paketprüfung entfernt die Qt-/Bibliothekspfade der Build-Umgebung aus dem Kindprozess, damit fehlende Plugins nicht durch die installierten Entwicklungspakete verdeckt werden. FUSE-unabhängige Prüfung erfolgt mit `APPIMAGE_EXTRACT_AND_RUN=1`. Reguläres FUSE-Mounting und Auslieferung auf dem Zielsystem sind noch separat abzunehmen.

Der Workflow `.github/workflows/unstable.yml` läuft ausschließlich für `unstable`, baut unter Ubuntu 24.04, testet und lädt erst nach erfolgreicher Paketprüfung ein kurzlebiges Artefakt hoch. Es gibt keine GitHub-Release-Aktion, keine Schreibberechtigung für Repository-Inhalte und keinen automatischen Stable-Publish. Auf `main` läuft dieser Workflow auch bei manuellem Start nicht.

Der Workflow wurde nach Upload-Freigabe erfolgreich auf GitHub unter Ubuntu 24.04 / Qt 6.4.2 ausgeführt: [Lauf 40938184412](https://github.com/TimBooH/VELUNO/actions/runs/40938184412), einschließlich isoliertem Paketstart und [Artefakt-Upload](https://github.com/TimBooH/VELUNO/actions/runs/40938184412/artifacts/12924389858). Das bestätigt noch keine CachyOS-Abnahme. Mindestglibc, Zielhardware, FUSE, Codec-/GPU-Abhängigkeiten und Drittanbieter-Lizenzen bleiben Teil der späteren Distributionsabnahme (O-004, O-010).
