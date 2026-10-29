# Architektur der technischen Grundlage

`src/app` verdrahtet den Lebenszyklus. QML kennt ausschließlich `AppSettings`, keine SQL-Verbindung oder mpv-Handles. Das reguläre ApplicationWindow nutzt Systemdekorationen und erzwingt weder X11 noch rahmenloses Fensterverhalten. Qt wählt das Plattform-Backend aus der Desktop-Sitzung.

| Schicht | Verantwortung / aktueller Umfang |
| --- | --- |
| qml | Feste obere Navigation, leere Bibliotheksseiten, Profilmenü, Einstellungen, Dark/Light-Farben |
| core | Theme-Auswahl, Persistenzfehler mit Rückmeldung an die Oberfläche |
| database | Verbindungslebenszyklus, gebundene SQL-Parameter, SQLite-Migrationen |
| player | Eigener CMake-Target mit libmpv-Abhängigkeit und vorbereitetem RAII-Lebenszyklus; keine Wiedergabe und keine Geräteinitialisierung |
| scanner | Schnittstelle für Start/Pause/Fortsetzen/Abbruch; keine Dateizugriffe |
| metadata | Austauschbarer Providervertrag und expliziter Einwilligungszustand; kein Netzwerkclient |
| übrige Module | Reservierte Modulverzeichnisse mit dokumentierter Verantwortung |

Scanner- und Metadatenimplementierungen werden später auf Worker-Threads ausgeführt. Fortschritt und Ergebnisse werden über queued signals in die UI übergeben. Eine DB-Verbindung gehört jeweils ihrem erzeugenden Thread; spätere Worker erhalten eigene Verbindungen. Das jetzige Gerüst startet keine Hintergrundaufgaben.

## Datenbank / Migration 001

`profiles(id, name)` enthält genau ein initiales Standardprofil. `app_settings(key, value)` enthält `active_profile=1` und `theme=midnight`. Bibliothekstabellen werden mit ihren Fachmodulen in späteren Migrationen ergänzt, statt jetzt ein ungeprüftes vollständiges Medienschema festzulegen.

`PRAGMA user_version` steuert lückenlose Migrationen ab Version 1. Jede Version läuft atomar in einer Transaktion; bei Fehlern erfolgt Rollback. Bereits durchgeführte Migrationen werden übersprungen. Höhere Schemaversionen werden vor Änderungen abgelehnt. `foreign_keys=ON`, ein Busy-Timeout und `quick_check` sind aktiv. Es gibt keine automatische Datenbank-Neuanlage als Fehlerbehandlung und keinen automatischen Downgrade.

Die initiale Migration ist klein und läuft beim Start. Umfangreiche spätere Migrationen müssen vor ihrer Umsetzung auf Startzeit, Sicherung und Thread-Nutzung geprüft werden.

## Oberfläche und Umfang

Midnight Blue mit Cyan ist Standard, Light Mode ist auswählbar und persistent. Beide Designs verwenden dieselben semantischen Farbtokens einschließlich der Qt-Control-Palette. Tastaturfokus wird sichtbar dargestellt. Häufige Navigation hat keine verzögernde Animation.

Leere Startseitenreihen werden ausgeblendet; ohne Bibliothek zeigt die Startseite einen kompakten Leerzustand. Es gibt keinen Hero-Banner, keine Demofilme und keine vorgetäuschte Wiedergabe. Suche ist sichtbar vorbereitet und deaktiviert. Weitere Einstellungsbereiche sind ausdrücklich als späterer Entwicklungsschritt gekennzeichnet. Der Einrichtungsassistent bleibt ausstehend; es wird keine Online-Einwilligung angenommen.
