# Anforderungsstatus · 0.1.0-dev

Alle Anforderungen bleiben erhalten. „Geplant“ bedeutet nachgelagerte Implementierung, keine Streichung. Der Gesamtauftrag vom 30.10.2026 umfasst alle fehlenden Softwarebereiche. Zugänge und Updatequelle werden nach Nutzerentscheidung später in den Einstellungen hinterlegt. Die Zielsystemabnahme ist nicht abgeschlossen; Stable v0.1.0 wurde am 31.10.2026 ausdrücklich freigegeben und veröffentlicht.

Die ausdrücklichen Entwicklungsfreigaben ersetzen den Planungsstatus des PDFs für die implementierten Bereiche. Die ausdrückliche Freigabe vom 31.10.2026 erlaubt ausschließlich die Veröffentlichung von v0.1.0 und die Übernahme nach main; spätere Stable-Versionen benötigen eine neue Freigabe. CMake ist durch den Auftrag bestätigt.

| ID | Anforderung | Status dieser Stufe |
| --- | --- | --- |
| G-001 | Native Desktop-Anwendung | Eigenständiges Fenster umgesetzt; KWin-/HiDPI-Zielabnahme offen |
| G-002 | Originaldateien erhalten | Import, Player, Profil-/Sammlungsaktionen und Sicherung schreiben keine Originalvideos; Hash-Tests bestanden |
| G-003 | Offline-Betrieb | Bibliothek, persönliche Daten, gespeicherte Bilder und Wiedergabe lokal verfügbar; Onlinefunktionen optional |
| G-004 | Keine Programmierung ohne Freigabe | Prozessregel berücksichtigt |
| UI-001 | Designs | Beide Designs über gemeinsame semantische Palette; UI-/Screenshotprüfung, Zielabnahme offen |
| UI-002 | Feste obere Navigation | Alle fünf Navigationsbereiche funktionsfähig; Suche/Profilmenü rechts, Einstellungen im Profilmenü |
| UI-003 | Startseite ohne großen Hero-Banner | Weiterschauen, Zuletzt hinzugefügt, Meine Filme, Meine Serien in Sollreihenfolge; leere Reihen verborgen |
| UI-004 | Kacheln und Scrollverhalten | Querformat-Fortschrittskacheln und horizontale Reihen, Posterraster; große Bibliotheken/HiDPI offen |
| UI-005 | Zufällige Startseitenreihen | Stabile Zufallsreihen pro Sitzung für Filme und Serien umgesetzt |
| UI-006 | Interaktive Poster und Detailseiten | Poster/Backdrop, Hover/Play, Metadaten, geschützte Bilderwahl und Versions-/Episodendetails implementiert |
| UI-007 | Bibliotheksübersichten | Film-/Serienraster mit Titel/Jahr/Neu-Sortierung und Verfügbarkeitsfilter umgesetzt |
| LIB-001 | Mehrere Medienordner | Mehrere lokale und SMB-Film-/Serienordner über KDE, sichere Deaktivierung umgesetzt; echtes NAS ungetestet |
| LIB-002 | Import und Beobachtung | Rekursiver Import, Startprüfung, Dateibeobachtung und 60-s-Rückfallprüfung umgesetzt; reale Mount-Abnahme offen |
| LIB-003 | Automatische Erkennung | Lokale Namensheuristik/SxxEyy plus optionale TMDB-Zuordnung; echter Anbieterzugang noch einzurichten |
| LIB-004 | Nicht zugeordnet | Unklare Dateien separat; manuelle Titel-/Jahr-/Episodenzuordnung umgesetzt |
| LIB-005 | Mehrere Filmversionen | Mehrere Dateien pro Film mit echter Auflösung und Auswahl umgesetzt |
| LIB-006 | Nicht verfügbare Laufwerke | Offline-Einträge erhalten und Wiederkehr erkannt; simuliert getestet, auch über KIO-FUSE-Vertrag |
| LIB-007 | Hintergrundscan | Worker-Scan, Fortschritt, inkrementelle Speicherung und Pause/Fortsetzen umgesetzt |
| META-001 | Einwilligung zu Online-Abfragen | API-Zugang UND ausdrückliche Zustimmung erforderlich; Widerruf verwirft Warteschlange; Netzwerk-Test bestanden |
| META-002 | Automatische und manuelle Zuordnung | Eindeutige Titel-/Jahrestreffer automatisch; manuelle Trefferauswahl/Korrektur implementiert |
| META-003 | Lokaler Informationsbestand | Beschreibung/Genre/Bewertung und mehrere lokale Poster-/Backdrop-Varianten; echte TMDB-Abnahme offen |
| META-004 | Manuelle Änderungen schützen | Manuelle Titel/Beschreibung und gewählte Bilder geschützt; automatisierter Aktualisierungstest bestanden |
| META-005 | Automatische Aktualisierung | Stündliche Fälligkeitsprüfung, Aktualisierung nach sieben Tagen, Drosselung/429-Wartezeit implementiert |
| SEARCH-001 | Globale lokale Suche | Live-Suche ausschließlich lokale Filme/Serien; Episoden und Sammlungen entsprechend Pflichtenheft ausgeschlossen |
| PRO-001 | Mehrere lokale Profile | Mehrere lokale Profile, gemeinsame Bibliothek, Erstellen/Bearbeiten/Löschen implementiert und getestet |
| PRO-002 | Startprofil | Standardprofil bleibt; zuletzt aktives Profil in DB gespeichert |
| PRO-003 | Avatare | Standardavatare, farbige Initialen und kopierte eigene Bilder implementiert |
| PRO-004 | Persönliche Daten trennen | Favoriten/Sammlungen/Fortschritt/Gesehen/Ansichten durch profile_id getrennt; Isolationstest bestanden |
| PRO-005 | Sicherer Profilwechsel | Fortschritt speichern und Player stoppen vor Profilwechsel implementiert |
| COL-001 | Favoriten | Profilbezogene Favoriten setzen/entfernen und anzeigen implementiert |
| COL-002 | Persönliche Sammlungen | Mehrfachzuordnung, automatische Poster-Collage und eigenes Cover implementiert |
| COL-003 | Sammlungsordnung | Titel-/Hinzufügedatum-Sortierung implementiert; kein Drag-and-drop eingeführt |
| COL-004 | Löschen ohne Medienverlust | Nur persönliche DB-Einträge werden gelöscht; Originaldatei-Hashes geprüft |
| PLY-001 | Integrierte Wiedergabe | Integrierte libmpv-Wiedergabe und Vollbild; Softwarebildtest unter Wayland bestanden |
| PLY-002 | Player-Steuerung | Overlay mit 2-s-Ausblendung und ±10-s-Sprüngen implementiert und getestet |
| PLY-003 | Fortsetzen und Gesehen-Status | 5-s-/Pause-/Stop-/Schließen-Speicherung; Fortsetzen und 90-%-Neustartregel getestet |
| PLY-004 | Versionspriorität | Höchste tatsächliche Auflösung zuerst; alternative Version manuell bei Fehler wählbar |
| PLY-005 | Audio | Deutschpräferenz, Spur-/Gerätewahl und Lautstärkepersistenz implementiert; physische Ausgabe offen |
| PLY-006 | Untertitel | Untertitel zunächst aus, Spurwahl/externe Dateien/Größe/Position/Hintergrund/Versatz implementiert |
| PLY-007 | Wiedergabegeschwindigkeit | 0,5×–2,0× implementiert; API-Grenzen getestet |
| PLY-008 | Nächste Serienepisode | Benannte Credits-Kapitel, manueller Marker und Ende-Fallback; 10-s-Countdown/Abbrechen/Jetzt/letzte Folge implementiert |
| PLY-009 | Hardwarebeschleunigung | Automatisch/Hardware bevorzugen/Software konfigurierbar; Softwarebildausgabe aktiv, Hardware-/HDR-Abnahme offen |
| PLY-010 | Fehleranzeigen | Datei-/Codec-/Renderer-/SMB-Fehler sichtbar; keine Veränderung der Medien |
| PLY-011 | Tastatursteuerung | Alle Standardaktionen einschließlich N, persistente Neubelegung und Konfliktprüfung implementiert |
| WIZ-001 | Sechsstufiger Assistent | Alle sechs Schritte mit Sprach-/Profil-/Avatarwahl; nachträgliche Bildwahl in Profileinstellungen |
| WIZ-002 | Optionale Schritte | Ordner optional; Metadatenentscheidung ausdrücklich erforderlich |
| WIZ-003 | Startseite während Scan | Startseite während aktivem Scan erreichbar und laufend aktualisiert |
| WIZ-004 | Scanstatus und Steuerung | Fortschritt, Pfad, Zähler und Pause/Fortsetzen umgesetzt |
| WIZ-005 | Fehlerbereich | Persistente Fehlerliste mit quellbezogenem Wiederholen umgesetzt |
| WIZ-006 | Abschlussbenachrichtigung | Fensterstatus und asynchroner D-Bus-Aufruf; echte KDE-Zustellung ungetestet |
| WIZ-007 | Unterbrechungen | Assistentenschritt und Teilergebnisse bleiben erhalten; neuer Scan mit Cache nach Neustart getestet |
| SET-001 | Erscheinungsbild | Midnight Blue Standard und Light persistent, Deutsch-/Englisch-Auswahl ergänzt |
| SET-002 | Einstellungsbereiche | Alle Bereiche enthalten ihre Fachfunktionen; keine sichtbaren Bereichsplatzhalter |
| SET-003 | Bibliothekseinstellungen | Quellen, Beobachtung, Scanstatus und Metadatenentscheidung erreichbar |
| SET-004 | Player-Einstellungen | Audiogerät, Untertitel, Decoderwahl und Tastenkürzel erreichbar und gespeichert |
| BAK-001 | Tägliche Sicherung | Tagesprüfung bei Start und minütlich; sieben erfolgreiche Tagesstände; Rotation mit acht Snapshots getestet |
| BAK-002 | Manuelle Sicherungen | Manuelle Sicherungen ohne Rotation; Löschung nur nach expliziter Bestätigung |
| BAK-003 | Update-Sicherungen | Separate Sicherung vor bestätigter Installation; zusätzliche lokale Sicherung vor Migration |
| BAK-004 | Speicherort | Standardordner/anderes eingebundenes Ziel, Fehlerstatus; echte Wechseldatenträger/NAS noch offen |
| BAK-005 | Sicherungsumfang | Konsistenter DB-Snapshot plus alle referenzierten dauerhaften Bilder; keine Originalvideos |
| BAK-006 | Vollständige Wiederherstellung | Gesamtwiederherstellung beim Neustart, Vorsicherung und zurückgelegter Ordner; Rundlauf getestet |
| BAK-007 | Integritätsprüfung | SHA-256/Vollständigkeit/SQLite/Fremdschlüssel/Bildverweise geprüft; beschädigte Kopien abgewiesen |
| UPD-001 | Stable-/Unstable-Auswahl | Stable Standard, persistenter Kanalwechsel ohne automatische Installation |
| UPD-002 | Automatische Prüfung | Stündliche Prüfung konfigurierter signierter Quelle; ohne Quelle inaktiv, später durch Nutzer einzurichten |
| UPD-003 | Interner Download | Interner HTTPS-Download mit Fortschritt, Signatur-/Größen-/SHA-256-Prüfung implementiert |
| UPD-004 | Installation nur manuell | Eigene Bestätigung und erfolgreiche Update-Sicherung vor Austausch erforderlich |
| UPD-005 | Stable-Freigabe | Prozessregel berücksichtigt |
| UPD-006 | Unstable-Artefakte | Prozessregel berücksichtigt |
| UPD-007 | Spätere Unstable-Quelle | Client/Signaturformat/Quellkonfiguration und lokales Signierwerkzeug vorhanden; reale dauerhafte Quelle ausdrücklich ausstehend |
| UPD-008 | Sichere Installation | Kanal/Schema/glibc/Downgrade-Prüfung, atomarer Austausch und erhaltene Vorgängerdatei getestet; echte AppImage-Installation offen |
| TECH-001 | Modulare Schichten | UI, Fachcontroller, SQL, Netzwerk, Worker und libmpv getrennt; siehe Architektur |
| TECH-002 | Wayland-Unterstützung | Nativer Wayland-/Software-Compositor-Test bestanden; KWin/HiDPI/Mehrschirm offen |
| TECH-003 | Linux-Integration | Dateidialoge, Gerätewahl, D-Bus-Benachrichtigungen und KIO-FUSE integriert; reale KDE-Abnahme offen |
| TECH-004 | Datenbanksicherheit | Migrationen 1–4, WAL, Transaktionen, Konsistenzprüfung, Snapshots und Wiederherstellung implementiert |
| REL-001 | Ein Repository | Ein Repository mit main und unstable |
| REL-002 | Entwicklungszweig | Entwicklung ausschließlich auf unstable; getesteter v0.1.0-Stand nach ausdrücklicher Freigabe nach main übernommen |
| REL-003 | Testbuilds | Actions testet vor AppImage-Artefakt; aktueller Nachweis im Entwicklungsprotokoll |
| REL-004 | Release-Gate | v0.1.0 ausdrücklich freigegeben; Tag/main-Gleichheit, Tests, Paketprüfung und Draft-Assets vor Veröffentlichung geprüft |
| Q-001 | Datenintegrität | Medien-Hashes, Profilisolation, Snapshot-/Restore- und Updatefehlerfälle automatisiert geprüft |
| Q-002 | Responsivität | Scan/Backup/Bildverarbeitung/Installationskopie auf Worker; Netzwerk asynchron; Großbibliotheksabnahme offen |
| Q-003 | Fehlertoleranz | Defekte Dateien, Offline-Quellen, Abbruch, Metadatenfehler, beschädigte Sicherungen und ungültige Updates geprüft |
| Q-004 | Barrierearme Bedienung | Fokus/Standarddialoge/lesbare Themes; vollständige Screenreader-/HiDPI-Abnahme offen |
| O-001 | · Finale Produktbezeichnung / Markenprüfung | Arbeitsname bleibt; finale Markenprüfung weiterhin offen und in den Releasehinweisen ausgewiesen |
| O-002 | · Metadatenanbieter / Rechtliches | TMDB-Anbindung optional implementiert; aktuelle Bedingungen/Logo/Bildrechte und echter Zugang weiterhin offen; in den Releasehinweisen ausgewiesen |
| O-003 | · libmpv + Qt-Rendering | Offizielle Software-Render-API unter Wayland getestet; Zielhardware/HiDPI offen |
| O-004 | · Codec, HDR und GPU | Decoderwahl/Fallback konfigurierbar; echte GPU-, Codec- und HDR-Matrix offen |
| O-005 | · Abspannerkennung | Benannte Kapitel/manueller Marker/Ende-Fallback umgesetzt; keine behauptete Bildinhalts-Erkennung |
| O-006 | · Dauerhafte Unstable-Updatequelle | Nutzer hinterlegt später Quelle/öffentlichen Schlüssel; gehostete signierte Quelle nicht vorgetäuscht |
| O-007 | · AppImage-Installation/Rollback | Atomarer Dateiaustausch und alter Stand erhalten; produktive manuelle Rückkehr-/Datenkompatibilitätsabnahme offen |
| O-008 | · Backup-Details | Tagesprüfung/7er-Rotation, Mindestpuffer und Fehlerbehandlung festgelegt; manuelle/Updatekopien unbefristet |
| O-009 | · Datenbankschema und Migration | Schema 1–4 und Upgrade-Snapshot/Wiederherstellung entworfen und getestet |
| O-010 | · Plattformübergreifende Testmatrix | Lokaler Debian-13-/Qt-6.8- und Ubuntu-24.04-/Qt-6.4-CI-Pfad; AppImage-Baseline glibc 2.39; CachyOS-Hardwareabnahme offen |
| CTRL-001 | Änderungskontrolle | Prozessregel berücksichtigt |
| CTRL-002 | Keine stillschweigende Entwicklungsfreigabe | Prozessregel berücksichtigt |
| CTRL-003 | Keine stillschweigende Stable-Freigabe | Prozessregel berücksichtigt |

Die konkrete Nutzerfreigabe für Stable v0.1.0 ersetzt keine Zielsystem-, Anbieter- oder Updatequellenprüfung und erteilt keine Freigabe zukünftiger Stable-Versionen. Die verbleibenden Nachweise stehen einzeln in O-001 bis O-010 und in FULL_BUILD.md.
