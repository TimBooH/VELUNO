# platform

MediaLocation normalisiert SMB-Adressen ohne Zugangsdaten und verbindet sie über KIO-FUSE/D-Bus. Der Scanner wartet nur auf seinem Worker-Thread; GUI-Aufrufe sind asynchron. Persistierte Identitäten bleiben SMB-URLs, lokale FUSE-Pfade gehören nur zur laufenden Sitzung. Benachrichtigungen liegen derzeit im LibraryController. Weitere Portal-/Desktopintegration bleibt offen. Siehe [LIBRARY.md](../../docs/LIBRARY.md#smb-freigaben-auf-kde).
