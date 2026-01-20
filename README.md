Instrukcja Uruchomienia

1. Przygotowanie środowiska
Pobierz kod źródłowy projektu.

Upewnij się, że biblioteka SFML 3.0 jest poprawnie skonfigurowana w Twoim środowisku (ścieżki do include oraz lib).

2. Struktura folderów
Aby gra działała poprawnie obok pliku wykonywalnego .exe (lub w katalogu roboczym projektu) muszą znajdować się następujące foldery z zasobami:

animacje/ (pliki .png z klatkami animacji gracza i wrogów)

sprites/ (pliki .png tła, broni, ikon)

dzwieki/ (pliki .wav i .ogg z efektami dźwiękowymi i muzyką)

czcionka/ (plik arial.ttf)

3. Kompilacja
Otwórz plik rozwiązania (.sln) w Visual Studio.

Wybierz konfigurację Release x86

Zbuduj projekt Build Solution

Upewnij się, że pliki .dll biblioteki SFML (np. sfml-graphics-3.dll, sfml-window-3.dll, etc.) znajdują się w tym samym folderze co plik .exe.



STEROWANIE:


Klawisz                 Akcja
W / Strzałka W Górę     Skok
A / Strzałka W Lewo     Ruch w lewo
D / Strzałka W Prawo    Ruch w prawo
S / Shift,Sprint        Bieg

Myszka Obsługa menu i wybór ulepszeń
ESC                     Pauza / Wznowienie
F11                     Tryb pełnoekranowy (Fullscreen)
