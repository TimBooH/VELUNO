# Ausdrückliche Stable-Freigabe v0.1.0

Datum: 31.10.2026. Nutzerauftrag: „Mach die Version von unstable jetzt öffentlich in Main stable“.

Freigegeben ist die Übernahme des geprüften unstable-Funktionsstands nach main und die öffentliche Veröffentlichung als v0.1.0 mit AppImage. Ausgangsstand: f453dc2994ecf2179f60d555f381be499ed811a1; geprüfter Anwendungscode d40ac4c7343819ee0e20973ec032963a91de6e73, Actions-Lauf 41157032462 erfolgreich. Die Releasevorbereitung ändert Versions-/Paketkennungen, Dokumentation und den Veröffentlichungsablauf; keine weiteren Produktfunktionen.

Die offen dokumentierten Zielgerät-, Nutzerbibliotheks-, TMDB-, Anbieter-/Namens- und Updatequellenprüfungen werden nicht als erledigt markiert. Die Releasehinweise nennen diese Grenzen ausdrücklich.

Dies ist keine pauschale Freigabe weiterer Stable-Versionen. Der Releaseworkflow reagiert ausschließlich auf den manuell gesetzten Tag v0.1.0, verlangt denselben Commit auf main, baut und prüft erneut, lädt alle Assets zunächst in einen Draft und veröffentlicht erst nach erfolgreicher Vollständigkeits-/Prüfsummenprüfung. Ein normaler Push nach main oder unstable veröffentlicht keinen Stable-Release. Neue Versionen benötigen eine neue ausdrückliche Freigabe und eine entsprechende Erweiterung des Tag-Gates.
