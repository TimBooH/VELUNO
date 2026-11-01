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

[Geprüften Unstable-Build herunterladen](https://github.com/TimBooH/VELUNO/actions/runs/41196668508/artifacts/13041232704) (GitHub-Anmeldung erforderlich; verfügbar bis 14.11.2026, 12:27 Uhr Europe/Berlin). [Erfolgreicher Actions-Lauf](https://github.com/TimBooH/VELUNO/actions/runs/41196668508): alle acht Suiten sowie die AppImage-Prüfungen bestanden.

## Integrierte Fenstersteuerung · 31.10.2026

Der Nutzer hat die separate System-Titelleiste ausdrücklich abgewählt. Das native Hauptfenster verwendet jetzt einen rahmenlosen Fenstermodus mit Fenstersteuerung in der Navigation.

| Before | After | Why |
| --- | --- | --- |
| System-Titelleiste über dem VELUNO-Header | Minimieren, Maximieren/Wiederherstellen und Schließen im gemeinsamen VELUNO-Header | Ein zusammenhängendes Design in beiden Themes |
| Systemrahmen als einzige Verschiebe-/Resizefläche | Ziehen an freien Headerflächen, Doppelklick und acht native Resize-Ränder/-Ecken | Die Fenstergesten bleiben ohne separate Titelleiste erreichbar |
| Feste Headerabstände | Kompakte Abstände und Wortmarke bei kleinen Fenstern | Navigation, Suche, Profil und Fensterknöpfe passen weiterhin bei 980×600 |

[Midnight Blue bei 980×600](screenshots/integrated-window-midnight.png) · [Light Mode bei 980×600](screenshots/integrated-window-light.png). Aufnahmen aus der laufenden Qt-Anwendung mit synthetischen Testdaten. Das Fenster bleibt ein reguläres Linux-Anwendungsfenster; die eigentlichen Move-/Resize-Operationen werden über Qt an den Wayland-Compositor delegiert. KWin-/HiDPI-/Mehrschirm- und echte Pointergesten bleiben Zielgerätetests.

[Geprüfter Unstable-Build mit integrierter Fenstersteuerung](https://github.com/TimBooH/VELUNO/actions/runs/41200798935/artifacts/13042876173) (GitHub-Anmeldung, verfügbar bis 14.11.2026 um 13:25 Uhr Europe/Berlin). Alle acht Testsuiten und AppImage-Prüfungen bestanden; Details im Entwicklungsprotokoll. Enthält auch die vorherige Pixel-/Deckkraftkorrektur des Players.

## Querformatreihen nach Videovorlage · 01.11.2026

Explizite Nutzerentscheidung zur Abweichung von der bisherigen UI-004-Darstellung: **„Startseitenreihen wie im Video in Querformat; Übersichten weiter in Hochformat“**. Die vier vereinbarten Reihen und die Startseite ohne großen Hero bleiben erhalten. Beide vorhandenen Themes und die integrierte Fenstersteuerung werden weiterverwendet.

| Before | After | Why |
| --- | --- | --- |
| Hochformatposter auch in den Startseitenreihen | 16:9-Karten mit lokaler Hintergrundgrafik und eingeblendetem Titel | Die Bildwirkung und Reihendichte entsprechen der gezeigten Browse-Vorlage |
| Große Zeilenhöhe, Textzeile unter jedem Bild | Kleinere Reihenüberschriften, knappe Zwischenräume und Titel im Bild | Mehr Sammlung ist gleichzeitig sichtbar |
| Kleine Wiedergabeaktion beim Hover | Zeitverzögerte, schwebende Vorschau mit Wiedergabe, Favorit und Details | Direkte Aktionen ohne Abschneiden durch die Reihe |
| Eigene Abmessungen für Weiterschauen | Gleiche Kartenbreite und Bildproportion, Fortschritt am unteren Rand | Konsistente Reihen mit sichtbarem persönlichen Fortschritt |
| Feste Bildwahl | Hintergrund → Poster → lesbare Titeldarstellung | Fehlende Bilder verhindern die lokale Nutzung nicht |

[Startseite / Midnight Blue](screenshots/reference-home-midnight.png) · [Startseite / Light Mode](screenshots/reference-home-light.png) · [Hover / Midnight Blue](screenshots/reference-hover-midnight.png) · [Hover / Light Mode](screenshots/reference-hover-light.png).

Die Startseitenbilder sind selbst erzeugte Testmotive in einer temporären Bibliothek; sie illustrieren die laufende QML-Oberfläche und werden nicht als Filme oder fremde Poster mitgeliefert. Der Hover-Screenshot stammt aus dem Einzelreihentest. In der echten Bibliothek erscheinen vorhandene lokale/TMDB-Bilder. Film-/Serien-/Favoritenraster bleiben hochformatig. Die Umsetzung ergänzt keine Netflix-Inhalte, Empfehlungsranglisten oder weitere Startseitenkategorien. Prüfungen und verbleibende Zielgeräteabnahme stehen im Entwicklungsprotokoll.

[Geprüfter Unstable-Build mit Querformatreihen und Hovervorschau](https://github.com/TimBooH/VELUNO/actions/runs/41347936927/artifacts/13099399169) (GitHub-Anmeldung erforderlich, verfügbar bis 15.11.2026 um 21:42 Uhr Europe/Berlin). [Erfolgreicher Build-/Paketnachweis](https://github.com/TimBooH/VELUNO/actions/runs/41347936927): alle acht Suiten sowie AppImage-Start, Import und Ordnerdialog bestanden.
