# Abnahme auf CachyOS / KDE Plasma / Wayland

Diese Liste dokumentiert noch ausstehende Tests auf dem echten Zielgerät. Cloudtests verwenden synthetische Videos, einen lokalen TMDB-Testserver und einen simulierten KIO-FUSE-Dienst; sie ersetzen die folgenden Nachweise nicht. Die im Nutzerscreenshot gezeigten 62 Scanfehler und 76 nicht zugeordneten Dateien wurden nicht anhand der tatsächlichen Dateien untersucht und werden deshalb nicht als behoben ausgewiesen.

| Bereich | Durchführung auf dem Zielgerät | Erwartung / noch offen |
| --- | --- | --- |
| SMB | Freigabe in Dolphin anmelden, VELUNO-Ordner scannen, NAS trennen/wieder verbinden | Einträge bleiben erhalten, Fehler nennt betroffenen Pfad, erneuter Scan stellt Zugriff wieder her. Reales NAS offen. |
| Erkennung | Beispiele S01E02, 1x02, Staffel/Folge sowie echte problematische Dateinamen prüfen | Titel/Staffel/Folge vergleichen; unklare Dateien manuell zuordnen. Konkrete Nutzerbibliothek offen. |
| TMDB | Eigenen Lesezugriffstoken speichern, einschalten, Zugang prüfen, Bilder abrufen, dann ausschalten/Internet trennen | Richtige Treffer auswählen, lokale Beschreibung/Poster bleiben erhalten, keine Onlineabrufe bei Aus. Live-Zugang/Anbieterabnahme offen. |
| Fortschritt | Film pausieren/beenden, App neu starten; Serie in Staffel 2 beginnen | Titeldetails zeigen Fortsetzen; richtige Position/Folge und persönliche Daten pro Profil. Automatisiert mit Testmedien geprüft; echte Bibliothek offen. |
| Fenster/Bedienung | 980×600 und größere Fenster, beide Themes, Tastatur, Skalierung 100/150/200 %, Vollbild | Hauptaktionen, X/Escape, Scrollen, Menüs und Dialoge bleiben erreichbar. KWin/HiDPI/Mehrschirm offen. |
| Ton/Untertitel | Deutsches und alternatives Audio, reale Ausgabegeräte, eingebettete und externe Untertitel | Hörbarer Ton, korrekte Spur/Gerätewahl und Timing; neue Datei beginnt ohne Untertitel. Reale Geräte offen. |
| GPU/Codec/HDR | Eigene H.264/HEVC/AV1-Dateien und HDR-Material mit Auto-/Softwaremodus abspielen; Player → Wiedergabedetails ansehen | Tatsächlichen hwdec-current-Wert und Bild-/Tonfehler erfassen. Renderverfahren bleibt Software-Render-API; HDR-Farbtreue und Treiberunterstützung nicht bestätigt. |
| Originaldateien | Dateigröße/Prüfsumme vor und nach Import, Abspielen und Zuordnung vergleichen | VELUNO verändert keine Originalmedien. Mit synthetischen Dateien automatisiert geprüft. |

Für Fehler sind der genaue Text unter Einstellungen → Bibliothek, der betroffene Dateiname (ohne Zugangsdaten) und die Werte unter Player → Wiedergabedetails hilfreich. Diagnoseangaben sind keine automatische Freigabe für einen Stable-Release.

## Nachprüfung der Nutzeraufnahme vom 31.10.2026

Den neuen Unstable-Build mit derselben Datei (gegebenenfalls über SMB) starten: Das Videobild muss deckend bleiben, Grau-/Hauttöne dürfen nicht durch UI-Farben aufgehellt werden. Mehrfach Bibliothek → Fortsetzen → Zurück wechseln, pausieren, Vollbild und Fenstergröße wechseln. Hinter dem Player darf keine Bibliothek durchscheinen; nach Zurück darf kein Videorest sichtbar bleiben. Die Puffer-/Qt-Paint-Ursache ist automatisiert nachgewiesen und korrigiert; die konkrete Aufnahme und Hardware sind hier nicht als vollständig nachgetestet markiert. Bei verbleibender Überhellung zusätzlich Codec, Hardwaredecoder und HDR-/Farbraum des betreffenden Mediums prüfen.
