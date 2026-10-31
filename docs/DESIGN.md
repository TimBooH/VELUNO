# Streaming-Ansicht · Überarbeitung auf unstable

Am 31.10.2026 beauftragt und präzisiert: **Startseite ohne Banner; großes Motiv in den Filmdetails**. Die Reihenfolge Weiterschauen, Zuletzt hinzugefügt, Meine Filme, Meine Serien sowie Hochformatposter und beide Themes bleiben gemäß UI-001 bis UI-006 erhalten. Native Qt-/QML-Oberfläche mit eigener VELUNO-Gestaltung.

| Before | After | Why |
| --- | --- | --- |
| Helle blaue Flächen und zusätzliche Jahres-/Typzeile unter jedem Poster | Dunkleres Midnight Blue, eine Titelzeile, engere Posterabstände | Bilder und Titel bestimmen die Reihen |
| Gleich große Textaktionen unter den Filminformationen | Weiße Wiedergabeaktion und runde Favoriten-/Gesehen-/Sammlungsaktionen direkt am Motiv | Die wichtigste Aktion ist sofort erreichbar |
| Kleinere Titel und weniger ausgeprägte Motivwirkung | Größere Titel über vollflächigem Motiv mit Verlauf und dunklerem Hintergrund hinter dem Dialog | Informationen sind direkt dem Film zugeordnet |
| Großes Leerfeld ohne Hintergrundbild bzw. im kleinen Fenster | Kleinerer Motivbereich ohne Bild und an die Fensterhöhe angepasste Darstellung | Mehr Platz für Beschreibung und Episoden |
| Animiertes Blättern auch per Tastatur | Sofortiges Blättern per Pfeiltaste, Pos1 und Ende; kurze Bewegung nur per Maus | Wiederholte Tastatureingaben reagieren unmittelbar |

Die Symbolknöpfe besitzen ausgeschriebene zugängliche Namen, Tooltips, Tastaturfokus und sichtbaren Zustand. Dialog-Schließen über X, Escape und Außenklick bleibt vorhanden. Datei-/Versionsauswahl, Zuordnung, Metadatenbearbeitung, Fortschritt und Episodensteuerung bleiben erreichbar. Bilder werden aus vorhandenen lokalen Metadaten geladen; ohne eigenen TMDB-Zugang bzw. eigene Bilder werden keine echten Cover erfunden.

## Geprüfte Ansichten

Die folgenden Screenshots stammen aus der laufenden Qt-6.4.2-Anwendung im Oberflächentest. „Example Movie“, „Orbit“ und die synthetische Landschaft sind ausdrücklich Testdaten und gehören nicht zur ausgelieferten Medienbibliothek.

- [Filmdetails / Midnight Blue](screenshots/design-title-midnight.png)
- [Filmdetails / Light Mode](screenshots/design-title-light.png)
- [Seriendetails / 980×600 ohne Hintergrundbild](screenshots/design-series-compact.png)

Automatisiert geprüft: Öffnen/Schließen, echtes Fortsetzen im integrierten Player, Favoritenschalter mit Maus und Leertaste, Filmversion/Zuordnung, Serien-/Staffelwahl, Sammlungs- und Metadatenaktionen, defektes Poster, horizontale Navigation und beide Designs. Vollständige Testergebnisse und GitHub-Paketnachweis im Entwicklungsprotokoll. KWin/HiDPI und reale Nutzerbibliothek bleiben Zielgerätetests.
