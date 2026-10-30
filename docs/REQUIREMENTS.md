# Anforderungsstatus · 0.1.0-dev

Alle Anforderungen bleiben erhalten. „Geplant“ bedeutet nachgelagerte Implementierung, keine Streichung. Die aktuelle Freigabe umfasst zusätzlich Player und lokale Bibliothek mit Hintergrundscan. Zielsystem- und Stable-Abnahmen sind nicht abgeschlossen.

Die ausdrückliche Entwicklungsfreigabe vom 29.10.2026 ersetzt den Planungsstatus des PDFs für diesen ersten Auftrag. Die Stable-/Veröffentlichungssperre bleibt bestehen. CMake ist durch den Auftrag bestätigt.

| ID | Anforderung | Status dieser Stufe |
| --- | --- | --- |
| G-001 | Native Desktop-Anwendung | Teilweise umgesetzt / vorbereitet; siehe Entwicklungsprotokoll |
| G-002 | Originaldateien erhalten | Import/Scan verändern keine Medien; SHA-256-Vergleich synthetischer Dateien bestanden |
| G-003 | Offline-Betrieb | Lokale Bibliothek und Wiedergabe ohne Onlineanbieter umgesetzt; weitere Module offen |
| G-004 | Keine Programmierung ohne Freigabe | Prozessregel berücksichtigt |
| UI-001 | Designs | Teilweise umgesetzt / vorbereitet; siehe Entwicklungsprotokoll |
| UI-002 | Feste obere Navigation | Teilweise umgesetzt / vorbereitet; siehe Entwicklungsprotokoll |
| UI-003 | Startseite ohne großen Hero-Banner | Startseite mit zuletzt hinzugefügt und Zufallsreihen; Fortsetzen folgt |
| UI-004 | Kacheln und Scrollverhalten | Raster, horizontale Reihen und profilbezogene Kachelgröße umgesetzt; Großbibliothek-Abnahme offen |
| UI-005 | Zufällige Startseitenreihen | Stabile Zufallsreihen pro Sitzung für Filme und Serien umgesetzt |
| UI-006 | Interaktive Poster und Detailseiten | Posterplatzhalter mit Hover/Play und Versions-/Episodendialog; Cover und vollständige Details offen |
| UI-007 | Bibliotheksübersichten | Film-/Serienraster mit Titel/Jahr/Neu-Sortierung und Verfügbarkeitsfilter umgesetzt |
| LIB-001 | Mehrere Medienordner | Mehrere lokale und SMB-Film-/Serienordner über KDE, sichere Deaktivierung umgesetzt; echtes NAS ungetestet |
| LIB-002 | Import und Beobachtung | Rekursiver Import, Startprüfung, Dateibeobachtung und 60-s-Rückfallprüfung umgesetzt; reale Mount-Abnahme offen |
| LIB-003 | Automatische Erkennung | Lokale Namensheuristik inklusive SxxEyy umgesetzt; Anbieterzuordnung offen |
| LIB-004 | Nicht zugeordnet | Unklare Dateien separat; manuelle Titel-/Jahr-/Episodenzuordnung umgesetzt |
| LIB-005 | Mehrere Filmversionen | Mehrere Dateien pro Film mit echter Auflösung und Auswahl umgesetzt |
| LIB-006 | Nicht verfügbare Laufwerke | Offline-Einträge erhalten und Wiederkehr erkannt; simuliert getestet, auch über KIO-FUSE-Vertrag |
| LIB-007 | Hintergrundscan | Worker-Scan, Fortschritt, inkrementelle Speicherung und Pause/Fortsetzen umgesetzt |
| META-001 | Einwilligung zu Online-Abfragen | Ausdrückliche Entscheidung persistiert; noch keine Online-Abfragen |
| META-002 | Automatische und manuelle Zuordnung | Geplant; spätere Ausbaustufe / Abnahme |
| META-003 | Lokaler Informationsbestand | Geplant; spätere Ausbaustufe / Abnahme |
| META-004 | Manuelle Änderungen schützen | Manuelle lokale Titel-/Episodenzuordnung vor Scanüberschreiben geschützt; weitere Metadatenfelder folgen |
| META-005 | Automatische Aktualisierung | Geplant; spätere Ausbaustufe / Abnahme |
| SEARCH-001 | Globale lokale Suche | Lokale Film-/Serientitelsuche umgesetzt; Episoden und Sammlungen folgen |
| PRO-001 | Mehrere lokale Profile | Geplant; spätere Ausbaustufe / Abnahme |
| PRO-002 | Startprofil | Teilweise umgesetzt / vorbereitet; siehe Entwicklungsprotokoll |
| PRO-003 | Avatare | Geplant; spätere Ausbaustufe / Abnahme |
| PRO-004 | Persönliche Daten trennen | Geplant; spätere Ausbaustufe / Abnahme |
| PRO-005 | Sicherer Profilwechsel | Geplant; spätere Ausbaustufe / Abnahme |
| COL-001 | Favoriten | Geplant; spätere Ausbaustufe / Abnahme |
| COL-002 | Persönliche Sammlungen | Geplant; spätere Ausbaustufe / Abnahme |
| COL-003 | Sammlungsordnung | Geplant; spätere Ausbaustufe / Abnahme |
| COL-004 | Löschen ohne Medienverlust | Geplant; spätere Ausbaustufe / Abnahme |
| PLY-001 | Integrierte Wiedergabe | Grundlegende lokale Wiedergabe integriert; Zielsystem-/Hardware-Abnahme offen |
| PLY-002 | Player-Steuerung | Overlay mit 2-s-Ausblendung und ±10-s-Sprüngen implementiert und getestet |
| PLY-003 | Fortsetzen und Gesehen-Status | Geplant; spätere Ausbaustufe / Abnahme |
| PLY-004 | Versionspriorität | Höchste tatsächliche Auflösung zuerst; alternative Version manuell bei Fehler wählbar |
| PLY-005 | Audio | Sprachpräferenz, Lautstärke und Stumm umgesetzt; echte Audioausgabe/Gerätewahl offen |
| PLY-006 | Untertitel | Neue Datei startet ohne Untertitel; Spurwechsel vorhanden, Gestaltung/externe Auswahl offen |
| PLY-007 | Wiedergabegeschwindigkeit | 0,5×–2,0× implementiert; API-Grenzen getestet |
| PLY-008 | Nächste Serienepisode | Geplant; spätere Ausbaustufe / Abnahme |
| PLY-009 | Hardwarebeschleunigung | Automatische Decoderoption vorbereitet; Software-Renderbasis aktiv, GPU und manuelle Auswahl offen |
| PLY-010 | Fehleranzeigen | Datei-/Codec-/Renderfehler mit Hinweisen sichtbar; zusätzliche Fehlerfälle später |
| PLY-011 | Tastatursteuerung | Grundbelegung umgesetzt und teilweise per UI getestet; N/Neubelegung/Konfliktprüfung folgen |
| WIZ-001 | Sechsstufiger Assistent | Sechs persistierte Schritte umgesetzt; Sprachauswahl und Avatare noch offen |
| WIZ-002 | Optionale Schritte | Ordner optional; Metadatenentscheidung ausdrücklich erforderlich |
| WIZ-003 | Startseite während Scan | Startseite während aktivem Scan erreichbar und laufend aktualisiert |
| WIZ-004 | Scanstatus und Steuerung | Fortschritt, Pfad, Zähler und Pause/Fortsetzen umgesetzt |
| WIZ-005 | Fehlerbereich | Persistente Fehlerliste mit quellbezogenem Wiederholen umgesetzt |
| WIZ-006 | Abschlussbenachrichtigung | Fensterstatus und asynchroner D-Bus-Aufruf; echte KDE-Zustellung ungetestet |
| WIZ-007 | Unterbrechungen | Assistentenschritt und Teilergebnisse bleiben erhalten; neuer Scan mit Cache nach Neustart getestet |
| SET-001 | Erscheinungsbild | Teilweise umgesetzt / vorbereitet; siehe Entwicklungsprotokoll |
| SET-002 | Einstellungsbereiche | Teilweise umgesetzt / vorbereitet; siehe Entwicklungsprotokoll |
| SET-003 | Bibliothekseinstellungen | Quellen, Beobachtung, Scanstatus und Metadatenentscheidung erreichbar |
| SET-004 | Player-Einstellungen | Geplant; spätere Ausbaustufe / Abnahme |
| BAK-001 | Tägliche Sicherung | Geplant; spätere Ausbaustufe / Abnahme |
| BAK-002 | Manuelle Sicherungen | Geplant; spätere Ausbaustufe / Abnahme |
| BAK-003 | Update-Sicherungen | Geplant; spätere Ausbaustufe / Abnahme |
| BAK-004 | Speicherort | Geplant; spätere Ausbaustufe / Abnahme |
| BAK-005 | Sicherungsumfang | Geplant; spätere Ausbaustufe / Abnahme |
| BAK-006 | Vollständige Wiederherstellung | Geplant; spätere Ausbaustufe / Abnahme |
| BAK-007 | Integritätsprüfung | Geplant; spätere Ausbaustufe / Abnahme |
| UPD-001 | Stable-/Unstable-Auswahl | Geplant; spätere Ausbaustufe / Abnahme |
| UPD-002 | Automatische Prüfung | Geplant; spätere Ausbaustufe / Abnahme |
| UPD-003 | Interner Download | Geplant; spätere Ausbaustufe / Abnahme |
| UPD-004 | Installation nur manuell | Geplant; spätere Ausbaustufe / Abnahme |
| UPD-005 | Stable-Freigabe | Prozessregel berücksichtigt |
| UPD-006 | Unstable-Artefakte | Prozessregel berücksichtigt |
| UPD-007 | Spätere Unstable-Quelle | Geplant; spätere Ausbaustufe / Abnahme |
| UPD-008 | Sichere Installation | Geplant; spätere Ausbaustufe / Abnahme |
| TECH-001 | Modulare Schichten | Teilweise umgesetzt / vorbereitet; siehe Entwicklungsprotokoll |
| TECH-002 | Wayland-Unterstützung | Teilweise umgesetzt / vorbereitet; siehe Entwicklungsprotokoll |
| TECH-003 | Linux-Integration | Geplant; spätere Ausbaustufe / Abnahme |
| TECH-004 | Datenbanksicherheit | Migrationen 1–2 atomar, WAL, getrennte Thread-Verbindungen; Backup/Reparatur später |
| REL-001 | Ein Repository | Teilweise umgesetzt / vorbereitet; siehe Entwicklungsprotokoll |
| REL-002 | Entwicklungszweig | Teilweise umgesetzt / vorbereitet; siehe Entwicklungsprotokoll |
| REL-003 | Testbuilds | Teilweise umgesetzt / vorbereitet; siehe Entwicklungsprotokoll |
| REL-004 | Release-Gate | Prozessregel berücksichtigt |
| Q-001 | Datenintegrität | Scan verändert Medien nicht und erhält Offline-/manuelle Daten; weitere Fachmodule offen |
| Q-002 | Responsivität | Analyse im Worker, kurze Schreibtransaktionen und inkrementelle Modelle; große reale Bibliothek ungetestet |
| Q-003 | Fehlertoleranz | Defekte Dateien, fehlende Ordner und Scanunterbrechung geprüft; weitere Fehlerfälle offen |
| Q-004 | Barrierearme Bedienung | Teilweise umgesetzt / vorbereitet; siehe Entwicklungsprotokoll |
| O-001 | · Finale Produktbezeichnung / Markenprüfung | Geplant; spätere Ausbaustufe / Abnahme |
| O-002 | · Metadatenanbieter / Rechtliches | Geplant; spätere Ausbaustufe / Abnahme |
| O-003 | · libmpv + Qt-Rendering | Geplant; spätere Ausbaustufe / Abnahme |
| O-004 | · Codec, HDR und GPU | Geplant; spätere Ausbaustufe / Abnahme |
| O-005 | · Abspannerkennung | Geplant; spätere Ausbaustufe / Abnahme |
| O-006 | · Dauerhafte Unstable-Updatequelle | Geplant; spätere Ausbaustufe / Abnahme |
| O-007 | · AppImage-Installation/Rollback | Geplant; spätere Ausbaustufe / Abnahme |
| O-008 | · Backup-Details | Geplant; spätere Ausbaustufe / Abnahme |
| O-009 | · Datenbankschema und Migration | Geplant; spätere Ausbaustufe / Abnahme |
| O-010 | · Plattformübergreifende Testmatrix | Geplant; spätere Ausbaustufe / Abnahme |
| CTRL-001 | Änderungskontrolle | Prozessregel berücksichtigt |
| CTRL-002 | Keine stillschweigende Entwicklungsfreigabe | Prozessregel berücksichtigt |
| CTRL-003 | Keine stillschweigende Stable-Freigabe | Prozessregel berücksichtigt |

Die offenen technischen Prüfungen O-001 bis O-010 bleiben offen; das aktuelle Fundament klärt nur den minimalen SQLite-Schemaeinstieg. Renderintegration, GPU/HDR, Metadatenanbieter, Updatequelle, Backupdetails und Distributionsmatrix werden nicht durch Platzhalter als erledigt gewertet.
