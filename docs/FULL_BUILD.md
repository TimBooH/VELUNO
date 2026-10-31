# Funktionsausbau nach Gesamtauftrag vom 30.10.2026

Der Nutzer hat die Implementierung des gesamten fehlenden Funktionsumfangs autorisiert. TMDB-Zugang und dauerhafte Updatequelle sollen später in den Einstellungen hinterlegt werden. Das autorisiert keine Stable-Veröffentlichung und keine Änderung an main. Alle bisherigen Daten werden über Migrationen ergänzt; Quelldateien werden nicht verändert.

## Bedienung

- **Profile:** Profilmenü → Profil wechseln oder Einstellungen → Profile. Profile erstellen, benennen, Avatar/Farbe wählen und nach Bestätigung löschen. Mindestens ein Profil bleibt erhalten. Die Bibliothek ist gemeinsam; Favoriten, Sammlungen, Ansichtsoptionen, Fortschritt und Gesehen-Status gehören zum jeweiligen Profil. Ein Wechsel speichert die Position und stoppt den Player.
- **Favoriten und Sammlungen:** Titel öffnen und als Favorit markieren oder mehreren Sammlungen zuweisen. Sammlungen im Navigationsbereich erstellen, umbenennen, nach Titel/Hinzufügedatum sortieren und löschen. Cover sind eine automatische Poster-Collage oder ein eigenes lokales Bild. Keine dieser Aktionen löscht Medien.
- **Fortschritt:** Automatische Speicherung alle fünf Sekunden sowie bei Pause, Stop, Profilwechsel und Schließen. Filme setzen fort; ab 90 Prozent gilt der Eintrag als gesehen und startet anschließend wieder von vorn. Die Startseite zeigt zuerst die profilbezogene Reihe Weiterschauen, danach Zuletzt hinzugefügt, Meine Filme und Meine Serien. Episoden sind getrennt gespeichert; Versionen desselben Films verwenden gemeinsamen Fortschritt.
- **Serien:** Benannte Kapitel mit Credits-/Abspann-Titel werden aus libmpv gelesen. Zusätzlich kann die aktuelle Position manuell als Abspann markiert werden. Wenn kein Marker bekannt ist, beginnt der Countdown am Episodenende. Countdown: zehn Sekunden, Abbrechen oder Jetzt abspielen. Nach der letzten verfügbaren Episode gibt es keinen Nachfolger. Es wird keine zuverlässige automatische Bildinhaltsanalyse behauptet.
- **Player:** Einstellungen für Audiogerät, Decoderwahl (automatisch, Hardware bevorzugen, Software), Untertitelgröße, Position, Hintergrund und Zeitversatz. Spurwahl und externe Untertitel sind im Player erreichbar. Tastenkürzel einschließlich N sind änderbar; doppelte Belegungen und das reservierte Strg+O werden abgewiesen. Die Bildausgabe bleibt die getestete libmpv-Software-Render-API; Decoderoptionen ersetzen keinen GPU-/HDR-Nachweis.
- **Sprache:** Deutsch/Englisch im Assistenten und in Allgemein, persistent und ohne Neustart umschaltbar. Benutzerdefinierte Titel werden nicht übersetzt. Technische Laufzeitdiagnosen können weiterhin deutsch sein.

## Metadaten und Bilder

Unter Einstellungen → Bibliothek wird ein eigener **TMDB-API-Lesezugriffstoken (Bearer)** hinterlegt. Der eigene Schalter **TMDB-Metadaten** im selben Bereich schaltet den Abruf ein oder aus und speichert zugleich die ausdrückliche Online-Entscheidung. Zugang speichern allein schaltet den Abruf nicht ein. Ohne beide Voraussetzungen werden keine Titelabfragen ausgelöst. Ausschalten bricht laufende Abfragen ab und leert die Warteschlange; bereits gespeicherte Metadaten und Bilder sowie der Token bleiben erhalten. Erneutes Einschalten setzt den automatischen Abruf fort. Der Schalter übernimmt bestehende Entscheidungen aus dem Einrichtungsassistenten; es gibt keine zweite widersprüchliche Freigabe. Der Token wird nicht in URLs oder Meldungen ausgegeben. Er liegt in der nur für den Besitzer les-/schreibbaren SQLite-Datenbank und ist Bestandteil der privaten Datensicherungen.

Die Suche ist während der Eingabe weiterhin rein lokal und umfasst ausdrücklich **nur Filme und Serien**, keine Episoden oder Sammlungen (SEARCH-001). Eine gesonderte TMDB-Suche im Titeldialog bietet manuelle Trefferauswahl. Automatische Übernahme erfolgt bei genau einem nach Titel/Jahr eindeutigen Treffer. Automatische Aktualisierung prüft stündlich auf Daten, deren letzter Abruf mindestens sieben Tage zurückliegt; Abfragen sind gedrosselt, begrenzt und reagieren auf HTTP 429 mit Wartezeit.

Titelbeschreibung, Genre, Bewertung, mehrere Poster- und Hintergrundvarianten werden lokal gespeichert. Bilder werden auf einem Worker decodiert und atomar abgelegt. Explizite Bildwahl und manuelle Titel-/Beschreibungsänderungen werden durch weitere Abrufe nicht überschrieben. Eigene lokale Bilder funktionieren auch ohne Onlinezugang. Die App zeigt TMDB-Attribution; es werden kein API-Zugang und keine fremden Poster mitgeliefert.

Vor produktiver öffentlicher Auslieferung müssen die aktuellen Anbieterbedingungen, Logo-/Attributionsvorgaben, Bildrechte und der konkrete Nutzungszweck geprüft werden. Die Dokumentations-Webabfragen waren in dieser Arbeitsumgebung nicht verfügbar; daraus wird keine Lizenzfreigabe abgeleitet. Offizielle Anlaufstellen: https://developer.themoviedb.org/docs/faq und https://www.themoviedb.org/api-terms-of-use. Eine echte TMDB-Abnahme erfordert den später vom Nutzer hinterlegten Zugang.

## Datensicherung und Wiederherstellung

Snapshots verwenden SQLite VACUUM INTO statt einer Kopie der laufenden WAL-Dateien. Gesichert werden die vollständige Datenbank und alle dauerhaft referenzierten Bilder aus images/ und metadata/. Originalvideos, ausführbare Programmdateien und wiederbeschaffbare Update-Downloads sind ausgeschlossen. Das Manifest enthält SHA-256-Prüfsummen und die tatsächliche Schemaversion. Vor einer erfolgreichen Kennzeichnung werden Vollständigkeit, sämtliche Hashes, SQLite-Integrität und Fremdschlüssel geprüft. Fehlende referenzierte Bilder, Zusatzdateien, Pfadtraversierung und symbolische Links werden abgewiesen.

Bei laufender Anwendung wird kurz nach Start und danach jede Minute geprüft, ob für den lokalen Kalendertag bereits eine erfolgreiche Tageskopie vorliegt. Sieben erfolgreiche Tagesstände bleiben erhalten. Manuelle, Update- und Vor-Wiederherstellungs-Sicherungen unterliegen keiner automatischen Rotation. Standardziel liegt als VELUNO-backups außerhalb des aktiven Anwendungsdatenordners; ein eigener eingebundener Ordner ist wählbar. Bei Speicher-/Kopierfehlern wird der unvollständige Stand entfernt und als Fehler gemeldet. Ein Grundpuffer von 16 MiB plus DB-Größe wird vorab geprüft; tatsächliche Bildkopierfehler führen ebenfalls zum Abbruch.

Wiederherstellen ersetzt den **gesamten** VELUNO-Zustand. Nach expliziter Bestätigung erfolgen Validierung, vorsorgliche Sicherung des aktuellen Stands und Vorbereitung in einem Nachbarverzeichnis. Ein modaler Hinweis verhindert weitere Bearbeitung. Beim nächsten Start wird der vorbereitete Stand mit Rückfall auf den zurückgelegten Datenordner aktiviert. Ein Instanz-Lock verhindert parallele Anwendung während dieser Aktivierung. Das vorherige Datenverzeichnis bleibt zusätzlich erhalten. Vor Schema-Upgrades erstellt der Programmstart außerdem einen lokalen Update-Snapshot; ein Fehlschlag verhindert die Migration.

## Updates und Installationsschutz

Stable ist der voreingestellte Kanal. Stable-/Unstable-Manifestadresse und ein **öffentlicher Ed25519-Schlüssel (PEM)** werden in Einstellungen → Updates hinterlegt. Es ist absichtlich keine erfundene oder ablaufende Artefakt-URL fest verdrahtet. Ohne Quelle/Prüfschlüssel bleibt die automatische stündliche Prüfung inaktiv. Die Bereitstellung einer dauerhaften verifizierbaren Quelle ist vor produktiver Unstable-Nutzung weiterhin erforderlich; der Nutzer wollte sie später konfigurieren.

Ein Manifest enthält eine base64-codierte JSON-Nutzlast und deren Ed25519-Signatur. Vor einem Download werden Signatur, Kanal, Plattform/Architektur, Versions-/Buildstand, minimale glibc, Datenbankkompatibilität, HTTPS-URL, Größe und SHA-256 geprüft. Stable nimmt keine Vorabversionen an. Downloads laufen intern mit Fortschritt; abgebrochene oder falsche Pakete werden verworfen. Installation benötigt eine eigene Bestätigung und eine erfolgreiche separate Update-Sicherung. Nur innerhalb eines AppImage wird die aktuelle Datei ersetzt. Das neue Paket wird im gleichen Verzeichnis nochmals verifiziert, ausführbar gemacht und per POSIX rename atomar ausgetauscht; die bisherige Datei bleibt als .previous-* erhalten. Der Nutzer beendet die App und startet neu. Es gibt keinen stillen Installer und keinen Release-Publish.

Die frühere AppImage-Datei und der Update-Datensnapshot ermöglichen eine manuelle Rückkehr. Es wird kein unbeaufsichtigter Downgrade einer neueren Datenbank und kein automatischer Rollback nach beliebigen Startfehlern behauptet. Produktive Wechsel-/Rollback-Abnahme auf Zielhardware steht aus.

Build-Informationen sind über `veluno --build-info` und im Artefakt als build-info.json verfügbar. `packaging/create_update_manifest.py` erstellt signierte Manifeste ausschließlich lokal; es lädt nichts hoch. Beispiel mit eigenem Ed25519-Schlüssel:

```sh
python3 packaging/create_update_manifest.py VELUNO.AppImage \
  --private-key /sicherer/ort/update-private.pem \
  --url https://eigene-quelle.example/VELUNO.AppImage \
  --version 0.1.0-dev --channel unstable --schema 4 --min-glibc 2.39 \
  --build '<build aus build-info.json>' --output unstable.json
```

Private Schlüssel gehören weder ins Repository noch ins AppImage. Beispieladressen sind keine aktiven Updatequellen.

## Nachweise und verbleibende Abnahmen

Automatisiert werden Profile/Isolation/Originaldateien, Filmfortschritt/90-Prozent-Regel, Serienwechsel und Countdown, Tastenkonflikte, Backup-Rotation/Korruption/Wiederherstellung, Metadaten-Zustimmung/Schutz manueller Eingaben, signierte/manipulierte/inkompatible Updates und atomarer Dateiaustausch geprüft. Die Tests verwenden synthetische Medien, einen lokalen API-Testserver und für SMB einen nachgebildeten KIO-FUSE-Dienst. Die vorhandenen Scanner-, Datenbank-, QML- und nativen Wayland-Bildtests bleiben erhalten.

Nicht durch Cloudtests ersetzt werden: echte NAS-/KDE-Anmeldung, echte Audioausgabe und Geräte, Hardwaredecoder/GPU/HDR, KWin/HiDPI/Mehrschirm, echte TMDB-Zugänge, produktiv gehostete signierte Updatequelle und Austausch eines tatsächlich gestarteten AppImage auf Zielhardware. Marke, Anbieterbedingungen und Drittanbieterlizenzen bleiben vor Stable freizugeben. Eine Vollständigkeitsbehauptung für alle Zielsystemabnahmen wäre deshalb falsch.

Die begrenzten Paket-Smoke-Tests nutzen absichtlich einen mpv-Null-Audioausgang und prüfen keine physischen Geräte. Die normale Anwendung enumeriert die tatsächlichen Geräte. Das native Video-Renderverfahren wird separat in player_render mit libmpv geprüft.

## Startseite und Coverdarstellung

Die Startseite verwendet rahmenlose 2:3-Poster und 16:9-Weiterschauen-Kacheln, mit Reihenpfeilen, Hover-Wiedergabe und Fortschrittsbalken. Gemäß UI-003 gibt es keinen großen Hero-Banner. Ohne Cover wird der vollständige Titel als Ersatz angezeigt, kein einzelner Buchstabe. Ein defekter Bildcache verhindert die Bedienung nicht.

Fehlende Cover: **Cover einrichten → Einstellungen/Bibliothek** führt zur ausdrücklichen Online-Freigabe und zum eigenen TMDB-Lesezugriffstoken. Anschließend Metadaten aktualisieren; mehrdeutige Titel über die Titeldetails zuordnen. Eigene Poster/Hintergründe lassen sich ohne Onlineabruf wählen. Die App bringt keine vorinstallierten kommerziellen Filmcover mit. Die untere Scanstatuszeile öffnet denselben Bibliotheksbereich mit konkreten Scanfehlern und Wiederholen. Die Gestaltung allein repariert keine unerreichbaren NAS-Dateien oder falschen Dateinamen.

## Film- und Seriendetails

Klick auf ein Poster öffnet eine große Titeldetailansicht mit lokalem Hintergrundbild, Titel, Abspielen, Beschreibung und vorhandenen Metadaten. Favoriten, Gesehen-Status und Sammlungen sind direkt erreichbar. Bei Serien führt „Episoden auswählen“ zur Folgenliste; über die Staffelauswahl werden die zugehörigen Episoden gezeigt. Das X bleibt beim Scrollen erreichbar; Escape schließt ebenfalls.

Technische Dateipfade, Versionen und Dateizuordnung stehen unter **Dateien und Versionen**. Die höchste Auflösung bleibt Standard, andere Versionen sind weiterhin einzeln abspielbar. **Informationen und Bilder bearbeiten** öffnet den bisherigen lokalen Editor mit optionalen TMDB-Aktionen. Ohne Bilder wird ein neutraler Hintergrund verwendet. Die Startseite bleibt gemäß UI-003 ohne großen Hero-Banner.

## Verbesserungen an Erkennung, Metadaten, Bedienung und Wiedergabe

- **Erkennung:** zusätzlich zu S01E02 werden 1x02, Staffel 1 Folge 2 sowie Episode/Folge/E-Nummern in expliziten Staffelordnern erkannt. Staffelordner werden nicht selbst zum Serientitel. Zahlenfilme wie 1917 und Titel mit eingebetteten Jahreszahlen bleiben unterscheidbar; bei mehreren Jahresangaben wird die letzte mit vorhandenem Titelpräfix als Releasejahr verwendet. Das ist eine Heuristik, keine garantierte Zuordnung. Manuelle Zuordnung bleibt geschützt. SMB-Medienanalyse erhält bis zu 30 Sekunden; lokale Analyse weiterhin 10 Sekunden. Fehler nennen Hinweise für Zugriffsrechte, defekte/unvollständige Dateien und Netzwerkprobleme. Fehlerpfade sind markier-/kopierbar; Wiederholen prüft den zugehörigen Quellordner.
- **TMDB:** Zugang prüfen sendet bei eingeschaltetem TMDB und gespeichertem Token eine authentifizierte Konfigurationsabfrage. Ein abgewiesener Zugang stoppt die übrige Warteschlange, bis erneut versucht bzw. der Token korrigiert wird. Bibliotheks-/Zuordnungs-/Posterzahlen und ausstehende Aufgaben sind sichtbar. Fehlende Bilder können gezielt erneut geladen werden. API- und Bildabrufe teilen eine auf zwei aktive Verbindungen begrenzte Warteschlange. Ausschalten beendet beide Warteschlangen, vorhandene lokale Daten bleiben erhalten. Statistik und automatische Abfragen berücksichtigen nur Titel mit Bibliotheksdateien; persönliche Daten verwaister Titel werden dadurch nicht gelöscht.
- **Titeldetails:** Fortsetzen zeigt die gespeicherte Position und einen Cyan-Fortschrittsbalken. Serien setzen die zuletzt angefangene Folge des aktiven Profils fort, wenn eine Version verfügbar ist; andernfalls bleibt die Folgenauswahl möglich. Filme behalten die höchste Auflösung als Standard. Folgen zeigen Fortschritt/Gesehen-Status; Versionen weisen ihre Auflösung deutlicher aus.
- **Bedienung:** Metadatenaktionen und Playerkopf können im kleineren Fenster umbrechen. Titeldetails fokussieren beim Öffnen die Hauptaktion; Reihen unterstützen zusätzlich Pos1 und Ende, sobald der Fokus in der Reihe liegt. Ladeanzeigen und Aufgabenanzahl zeigen laufende Metadatenarbeit.
- **Player:** Wiedergabedetails zeigt die tatsächlich gemeldeten Decoder-/Video-/Audioeigenschaften und die verwendete Renderintegration. Nicht gemeldet bedeutet nicht erfolgreich beschleunigt. Die App verwendet weiterhin die libmpv-Software-Render-API; Hardware-Decodierung und Bildausgabe sind unterschiedliche Ebenen. Reale NAS-/Audio-/GPU-/HDR-/KWin-/HiDPI-Abnahme bleibt erforderlich; siehe TARGET_TESTS.md.
