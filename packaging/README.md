# AppImage / Unstable

`appimage.sh BUILDVERZEICHNIS AUSGABEVERZEICHNIS` paketiert einen vorher getesteten Build und prüft das Paket im Offscreen-Modus mit neuem Datenverzeichnis. Das Ausgabeverzeichnis muss frisch sein, damit keine Reste älterer Pakete übernommen werden. Das Skript lädt nichts hoch. `ffprobe` wird als eigenes Programm mit seinen Bibliotheksabhängigkeiten paketiert. Die isolierte Prüfung importiert ein synthetisches Video mit dem im Paket enthaltenen Scanner und erwartet einen Filmtitel ohne Fehler.

linuxdeploy und das Qt-Plugin werden per SHA-256 auf den am 29.10.2026 geprüften Inhalt festgelegt. Die verwendeten `continuous`-URLs können sich ändern; dann muss die Prüfsumme nach erneuter Prüfung bewusst aktualisiert werden. Eine abweichende Datei bricht die Paketierung ab.

Wayland-, Offscreen- und SQLite-Plugins werden ausdrücklich mitgenommen. Die Paketprüfung entfernt die Qt-/Bibliothekspfade der Build-Umgebung aus dem Kindprozess, damit fehlende Plugins nicht durch die installierten Entwicklungspakete verdeckt werden. FUSE-unabhängige Prüfung erfolgt mit `APPIMAGE_EXTRACT_AND_RUN=1`. Reguläres FUSE-Mounting und Auslieferung auf dem Zielsystem sind noch separat abzunehmen.

Der Workflow `.github/workflows/unstable.yml` läuft ausschließlich für `unstable`, baut unter Ubuntu 24.04, testet und lädt erst nach erfolgreicher Paketprüfung ein kurzlebiges Artefakt hoch. Es gibt keine GitHub-Release-Aktion, keine Schreibberechtigung für Repository-Inhalte und keinen automatischen Stable-Publish. Auf `main` läuft dieser Workflow auch bei manuellem Start nicht.

Der Workflow wurde nach Upload-Freigabe erfolgreich auf GitHub unter Ubuntu 24.04 / Qt 6.4.2 ausgeführt: [Lauf 40949745517](https://github.com/TimBooH/VELUNO/actions/runs/40949745517), einschließlich isoliertem Paketstart und [Artefakt-Upload](https://github.com/TimBooH/VELUNO/actions/runs/40949745517/artifacts/12930812787). Das bestätigt noch keine CachyOS-Abnahme. Mindestglibc, Zielhardware, FUSE, Codec-/GPU-Abhängigkeiten und Drittanbieter-Lizenzen bleiben Teil der späteren Distributionsabnahme (O-004, O-010).

Aktueller Bibliotheksstand: [Lauf 41069263260](https://github.com/TimBooH/VELUNO/actions/runs/41069263260) und [Artefakt 12984037864](https://github.com/TimBooH/VELUNO/actions/runs/41069263260/artifacts/12984037864). Alle fünf Testsuiten sowie isolierter AppImage-Start mit Import und paketiertem ffprobe bestanden unter Qt 6.4.2. Das Artefakt gehört zum Quellcommit `021b99d37367c2220fd8c21601eb9a5ac02c69cc`; spätere reine Dokumentationscommits verändern es nicht.

Zusätzlich zum Medienimport wird der Ordnerdialog im isolierten AppImage tatsächlich geöffnet und geschlossen (`--smoke-test --smoke-dialog`). `Qt.labs.folderlistmodel` wird dafür ausdrücklich eingebunden; QtQml.Models und QtQuick.Shapes gehören ebenfalls zu den CI-Laufzeitabhängigkeiten.

Abschließend geprüfter Stand einschließlich Ordnerdialog: [Lauf 41070627636](https://github.com/TimBooH/VELUNO/actions/runs/41070627636), [Artefakt 12983369990](https://github.com/TimBooH/VELUNO/actions/runs/41070627636/artifacts/12983369990), Quellcommit `0f2c2efba0ae96e693f8c5cfaaa89af5ab67c73e`. Alle fünf Testsuiten sowie Import und Ordnerdialog im isolierten AppImage bestanden. Dieser Stand ist dem oben dokumentierten früheren Artefakt vorzuziehen.

SMB-Ergänzung: [Lauf 41089751941](https://github.com/TimBooH/VELUNO/actions/runs/41089751941), [Artefakt 12993657178](https://github.com/TimBooH/VELUNO/actions/runs/41089751941/artifacts/12993657178), Quellcommit `8a5730a7e2b965f46de4ff6c8d668f3c658391c2`. Alle sechs Testsuiten und isolierten Paketprüfungen bestanden. Für SMB werden auf dem Zielsystem zusätzlich `kio-fuse` und `kio-extras` sowie eine funktionierende KDE-Sitzungsverbindung benötigt. Diese Hostdienste werden nicht ins AppImage gepackt. Ein echter NAS-/KDE-Anmeldetest bleibt offen.


## Gesamtfunktionsausbau

Der neue Stand testet acht Suiten. OpenSSL-TLS-Backend und libssl/libcrypto werden ausdrücklich mitgenommen; isolierte Pakettests verlangen ein funktionsfähiges TLS-Backend, Medienimport und geöffneten Ordnerdialog. Der begrenzte Starttest verwendet Null-Audio; echte Ausgabe ist Zielsystemabnahme. build-info.json liegt neben AppImage/Prüfsumme im Artefakt und enthält Version, UTC-Buildzeit und Schema.

create_update_manifest.py erzeugt ein Ed25519-signiertes Manifest ausschließlich lokal. Eine dauerhafte HTTPS-Quelle und der öffentliche Prüfschlüssel werden später vom Nutzer in der App hinterlegt. Das Werkzeug veröffentlicht nichts und besitzt keinen privaten Schlüssel im Repository. Details und Beispiel: FULL_BUILD.md.

Aktueller Gesamtfunktionsstand: [Lauf 41104725560](https://github.com/TimBooH/VELUNO/actions/runs/41104725560), [Artefakt 13001525493](https://github.com/TimBooH/VELUNO/actions/runs/41104725560/artifacts/13001525493), Quellcommit `739e1f9307a2526b6551935ba0b0b50576df08ba`. Alle acht Testsuiten, isolierte OpenSSL-TLS-Prüfung, Import und Ordnerdialog bestanden. Das ZIP enthält zusätzlich build-info.json. Ältere oben dokumentierte Artefakte gehören zum Verlauf und enthalten diesen Gesamtumfang noch nicht.

## Freigegebene Stable-Version 0.1.0

Die ausdrückliche Nutzerfreigabe ist in `docs/releases/APPROVAL-v0.1.0.md` dokumentiert. `stable.yml` reagiert ausschließlich auf den manuell gesetzten Tag `v0.1.0`, prüft dessen Gleichheit mit main und baut mit `VELUNO_STABLE_BUILD=ON`. Nach allen acht Testsuiten und den isolierten AppImage-Tests werden AppImage, Prüfsumme und Buildinformationen zunächst in einen Entwurf hochgeladen und auf Vollständigkeit geprüft; erst danach wird der Release öffentlich. Normale Branch-Pushes veröffentlichen keinen Stable-Release. Der Standard-Build und unstable.yml bleiben Entwicklungsbuilds. Weitere Versionsfreigaben sind nicht enthalten.

Veröffentlicht: [Stable v0.1.0](https://github.com/TimBooH/VELUNO/releases/tag/v0.1.0), [erfolgreicher Release-Lauf 41159123633](https://github.com/TimBooH/VELUNO/actions/runs/41159123633). Alle drei öffentlichen Asset-URLs wurden abgerufen; die AppImage-Prüfsumme stimmt. Vollständiger Nachweis im Entwicklungsprotokoll.
