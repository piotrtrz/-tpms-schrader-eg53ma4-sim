# Konfiguracja i uruchomienie

## Przed rozpoczęciem

1. Sprawdź zgodność konkretnego auta (VIN, oznaczenia czujników, diagnostyka lub
   odbiór). Nie polegaj wyłącznie na numerze z ogłoszenia.
2. Zmontuj prototyp z `HARDWARE.md`, wgraj firmware i otwórz UART 115200.
3. Wpisz `stay 300`. `info` pokazuje konfigurację aktywną, nie tylko zapisaną.
4. Pracuj na postoju. Sztuczne 260 kPa nie opisuje rzeczywistych opon.

Koła: `1=FL` przód lewy, `2=FR` przód prawy, `3=RR` tył prawy, `4=RL` tył lewy.
Komendy konfiguracyjne automatycznie zapisują NVS i weryfikują odczyt. Błąd NVS
trzeba rozwiązać przed dalszym testem, nie ignorować.

## Ścieżka A — Clone IDs (preferowana)

Użyj jej, jeśli wcześniej odczytano oryginalne ID i auto nadal je zna.

1. Zapisz cztery ID w postaci sześciu cyfr hex.
2. Wpisz kolejno (przykładowe wartości zastąp własnymi):

   ```text
   id 1 2FC871
   id 2 2EAAF9
   id 3 3495AC
   id 4 2FC780
   pressure all 260
   info
   ```

3. Sprawdź w `info`, że wszystkie ID i przypisania kół są właściwe. Duplikat,
   `000000`, `FFFFFF` lub wartość poza 24 bitami zostanie odrzucona.
4. Opcjonalnie `send` wymusi cykl wszystkich kół. W NORMAL urządzenie robi to
   również automatycznie; po restarcie konfiguracja pozostaje.
5. Potwierdź odbiór zewnętrznie. Znane ID **mogą** wyeliminować naukę, ale nie
   obiecujemy tego dla każdego stanu ECU.

Nie używaj przykładowych ID w pojeździe, jeśli mogą pokrywać się z innym
urządzeniem w pobliżu.

## Ścieżka B — New IDs + LEARN

### Przygotowanie

1. `ids new` generuje cztery unikalne ID. Można zamiast tego użyć czterech `id`.
2. `info` — zapisz ID na kartce.
3. Zapoznaj się z procedurą nauki dla konkretnej wersji nadwozia/wyposażenia.
   Ten dokument nie zgaduje menu, sekwencji pedałów ani położenia odbiornika.
4. Rozpocznij procedurę nauki w aucie zgodnie z instrukcją serwisową.

### Nauka z konsoli

1. `mode learn` — start od FL; tylko FL nadaje co 2 s.
2. `send` — natychmiastowy burst FL, bez zmiany koła.
3. Po wykonaniu kroku dla FL: `next` — FR; LED wskaże 2 mrugnięcia.
4. Powtórz `send`, potem `next` — RR, następnie `next` — RL.
5. Po ostatnim `next` urządzenie wraca do NORMAL. Kolejność to zawsze
   **FL → FR → RR → RL**.
6. Zakończ procedurę auta zgodnie z jej instrukcją i sprawdź odczyt diagnostyczny.

### Nauka jednym przyciskiem

- wejście: przytrzymaj ≥1,5 s;
- krótkie naciśnięcie: następne koło i natychmiastowy burst;
- wymuszenie bez zmiany koła: konsola `send`;
- wyjście przed końcem: ponownie ≥1,5 s albo `mode normal`.

Timeout 60 s na koło przerywa LEARN do NORMAL; limit całej sesji to 15 min.
Nie jest to automatyczny sukces nauki — trzeba zacząć od właściwego koła ponownie.

## SINGLE / diagnostyka

`mode single 2` nadaje tylko FR co 2 s. `send` wymusza FR, `next` przechodzi dalej.
`mode normal` kończy; automatyczny limit wynosi 5 min. Tryb nie zapisuje wyboru
koła w NVS i po utracie zasilania startuje bezpiecznie w NORMAL.

## Pozostałe ustawienia

| Komenda | Zakres / skutek |
|---|---|
| `pressure all 260` | 0–637,5 kPa, krok bajtu P ≈ 2,73 kPa (kalibracja wg auta, `PROTOCOL.md`) |
| `temp all 23` | −40…85 °C; w ramce °F |
| `interval normal 30` | interwał bez ACC, 5–86400 s |
| `interval ignition 30` | z aktywnym sprzętowym ACC |
| `interval parking 120` | bez aktywnego ACC |
| `rf freq 433.92` | 433,05–434,79 MHz; zmiana wymaga uzasadnienia |
| `rf power -10` | −30, −20, −15, −10, −6, 0, 5 dBm |
| `rf burst 3` | 1–8 ramek na koło |
| `rf gap 50` | 20–500 ms między ramkami |
| `flags 4C900070` | eksperyment; znaczenie bitów nie jest znane |
| `factory YES` | kasuje własną konfigurację i przywraca domyślną |

Parametry harmonogramu są eksperymentalne. Moc zaczyna się od −10 dBm; zwiększaj
tylko po pomiarze i z uwzględnieniem przepisów. Niski poziom zasilania emulatora
nie ustawia żadnej flagi czujnika.

## Po restarcie lub brownout

Firmware ponownie czyta NVS. Poprawna wersja/CRC i zakresy są wymagane. Uszkodzony
lub niezgodny wpis powoduje komunikat i powrót do domyślnych, a nie wysyłanie
częściowo uszkodzonej konfiguracji. Każdy deep sleep ma budzik; nie ma trybu
`PARKING_SHUTDOWN`, który mógłby zostawić auto bez transmisji do ręcznego wybudzenia.
