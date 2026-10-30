# Lokale Bibliothek

## Benutzung

Beim ersten Start führt ein sechsstufiger Assistent durch Begrüßung, Profilname, Medienordner, Metadatenentscheidung, Scan und Abschluss. Ordner dürfen ausgelassen werden. Die Metadatenentscheidung muss ausdrücklich getroffen werden; auch bei Zustimmung führt dieser Stand keine Online-Abfragen aus. Nach dem Scanstart kann die Startseite bereits benutzt werden. Der Assistent speichert seinen Schritt; Sprache ist derzeit Deutsch, Avatarwahl folgt später.

Weitere Ordner lassen sich über **Medienordner** oder **Profil → Einstellungen → Bibliothek** hinzufügen. Jeder Ordner gehört zu Filmen oder Serien. Unterordner werden rekursiv durchsucht. Überlappende Ordner derselben Kategorie sind zulässig und werden anhand kanonischer Dateipfade dedupliziert; überlappende Film-/Serienordner werden zur Vermeidung widersprüchlicher Zuordnungen abgelehnt. Entfernen deaktiviert nur die Quelle und erhält die Einträge. Mediendateien werden weder umbenannt noch verändert oder gelöscht.

Filme wie `Mein.Film.2020.1080p.mkv` werden aus dem Namen erkannt. Serien verwenden `Meine.Serie.S01E02.mkv`; bei `S01E02.mkv` unter einem Staffelordner dient dessen übergeordneter Ordner als Serientitel. Dies ist eine lokale Namensheuristik, keine verifizierte Anbieterzuordnung. Unklare Namen landen unter **Nicht zugeordnet**. Dort lassen sich Titel, Jahr und bei Serien Staffel/Episode manuell ergänzen. Manuelle Zuordnungen bleiben bei weiteren Scans erhalten.

Gleicher Titel/Jahr/Kategorie fasst Dateien zusammen. Filmdetails zeigen die Versionen mit tatsächlicher ermittelter Auflösung. Abspielen bevorzugt die höchste Auflösung; ist sie offline oder fehlerhaft, wird der Fehler angezeigt und eine andere Version kann ausdrücklich gewählt werden. Serien zeigen eine nach Staffel/Episode geordnete Dateiliste. Automatische nächste Episode folgt später.

Startseite, Film-/Serienraster und lokale Suche zeigen die importierten Titel. Unverfügbare Einträge bleiben sichtbar und sind filterbar. Sortierung, Verfügbarkeitsfilter und Kachelgröße werden profilbezogen gespeichert. Zufallsreihen behalten ihre Reihenfolge innerhalb der Sitzung. Cover sind ausdrücklich als fehlend gekennzeichnet; Anbieterinformationen und Coverdownloads folgen später. Suche umfasst bisher Film-/Serientitel, keine Episoden oder Sammlungen. Favoriten, Sammlungen und Fortsetzen sind weiterhin offen.

## Scan und Persistenz

`ScannerWorker` besitzt auf einem eigenen QThread eine eigene SQLite-Verbindung. Rekursive Discovery und `ffprobe` laufen außerhalb des GUI-Threads. Der mitgelieferte `ffprobe` wird im AppImage bevorzugt; beim Entwicklungsstart wird ansonsten PATH verwendet. Medienanalyse ist pro Datei auf ungefähr zehn aktive Sekunden begrenzt. Unveränderte erfolgreiche Dateien werden anhand Größe und Änderungszeit wiederverwendet. Beschädigte Dateien werden beim nächsten Scan erneut geprüft.

Jede Datei wird in einer kurzen Transaktion gespeichert. Modelle werden während des Scans aktualisiert. Pause/Fortsetzen arbeiten über threadsichere Kontrollpunkte; beim Schließen wird abgebrochen und der Worker beendet. Nach einem Neustart erfolgt ein neuer Scan mit bereits gespeicherten Ergebnissen und Fingerprint-Cache, keine Wiederaufnahme eines offenen Prozesses. Abbruch markiert noch nicht geprüfte Dateien nicht als verschwunden. Fehler bleiben bei Unterbrechung erhalten; erst nach vollständig durchlaufenem, lesbarem Quellordner werden nicht erneut aufgetretene Fehler entfernt.

Fehlende Laufwerke markieren ihre Dateien als unverfügbar und erhalten die Titel. Wiederanschließen wird bei der nächsten Prüfung erkannt. Ordnersymlinks werden nicht verfolgt, um Schleifen zu vermeiden; Dateisymlinks werden über den kanonischen Zielpfad dedupliziert. Dateiendungen: mkv, mp4, m4v, avi, mov, webm, mpeg, mpg, ts, m2ts, wmv, ogv.

QFileSystemWatcher überwacht entdeckte Unterordner. Änderungen werden 1,2 Sekunden gesammelt. Zusätzlich erfolgt bei aktiver Beobachtung alle 60 Sekunden eine Prüfung, auch für Wiederanschlüsse und nicht beobachtbare Ordner. Die Einstellung deaktiviert Beobachtung während des Betriebs; die Startprüfung bleibt erhalten. Das Hinzufügen weiterer Quellen während eines Scans plant einen Folgescan. Eine Desktop-Benachrichtigung wird asynchron über freedesktop D-Bus angefragt; ohne Sitzungsbus bleibt die Statusmeldung im Fenster verfügbar.

## Datenbank und Grenzen

Migration 2 ergänzt Quellen, Titel, Dateien, Scanfehler und Profilpräferenzen. Migration 1 und bestehende Einstellungen bleiben erhalten; WAL erlaubt getrennte GUI-/Worker-Verbindungen. Ein älterer Build mit Schema 1 kann die aktualisierte Datenbank nicht öffnen. Es gibt keinen automatischen Downgrade und noch keine Sicherungs-/Wiederherstellungsfunktion.

Automatisiert geprüft sind echte synthetische Medien, Auflösungsauswahl, rekursiver Import, Überlappungen, unbekannte/defekte Dateien, manuelle Zuordnung, Pause/Fortsetzen/Neustart, Offline-/Rückkehrsimulation, Dateibeobachtung, Profilpräferenzen und Originaldatei-Hashes. Die UI-Tests bedienen Raster, Suche, Player, Zuordnung und Assistenten. Echte KDE-Ordnerdialoge, Desktop-Benachrichtigungen, physische Wechseldatenträger/Netzlaufwerke, sehr große Bibliotheken und CachyOS/HiDPI sind noch nicht abgenommen.
