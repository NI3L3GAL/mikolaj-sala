# ATmega Bare-Metal CLI & Auto-Baud UART 🚀

Lekki, oparty na przerwaniach interfejs wiersza poleceń (CLI) dla mikrokontrolerów AVR (np. ATmega328P). Projekt został napisany w czystym języku C (bare-metal), całkowicie omijając framework Arduino. Udowadnia, jak zoptymalizować pracę procesora, pozbywając się blokujących funkcji typu `_delay_ms()`.

### 🚀 Główne funkcjonalności
* 🔄 **Auto-Baud Rate:** Mikrokontroler sam wykrywa prędkość terminala (57600, 38400, 19200, 9600 bps) na podstawie jednego znaku synchronizującego (`U`).
* ⚡ **Nieblokująca architektura:** Znaki odbierane są w tle przez przerwanie `USART0_RX_vect`. Pętla główna reaguje dopiero, gdy całe polecenie jest gotowe.
* ⏱️ **Asynchroniczny Timer:** Sprzętowy `Timer1` działa w trybie CTC, tworząc niezależny licznik czasu (podobnie do `millis()`). Pozwala to np. na miganie diodą w tle, podczas gdy CLI cały czas nasłuchuje.
* 🧠 **Lekki, autorski parser:** Zamiast ciężkiej biblioteki `<string.h>`, napisałem własne, zoptymalizowane funkcje (`strcomp`, `starts_with`), aby maksymalnie oszczędzać pamięć Flash.

### 🛠️ Technologie i architektura

**Hardware & Ekosystem**
* Mikrokontrolery z rodziny AVR (np. Arduino Uno)
* C (avr-gcc), obsługa rejestrów sprzętowych
* Biblioteki bazowe: `<avr/interrupt.h>`, `<avr/boot.h>`

**Kluczowe mechanizmy**
* Przerwania sprzętowe (ISR) do asynchronicznej obsługi zdarzeń
* Ręczna konfiguracja układów peryferyjnych (UART, Timery)
* Odczyt unikalnego ID procesora (Boot Signature)

### 💻 Dostępne polecenia CLI

Wszystkie polecenia muszą zostać zakończone znakiem nowej linii (klawisz Enter).

| Komenda | Opis |
| :--- | :--- |
| `LED on` / `LED off` | Włącza lub wyłącza diodę testową (podpiętą pod PB5). |
| `PING` | Zwraca sprzętowy numer seryjny układu (hex). |
| `BLINK <ms>` | Uruchamia sprzętowe miganie diody w tle co X milisekund (np. `BLINK 500`). Mikrokontroler się nie zawiesza i dalej odbiera komendy! |
| `BLINK off` | Wyłącza asynchroniczne miganie. |

### 📫 Jak to uruchomić (Quick Start)?
1. Wgraj program na swój mikrokontroler.
2. Otwórz terminal szeregowy (np. RealTerm, minicom, PuTTY).
3. Ustaw jedną z obsługiwanych prędkości (np. `57600`).
4. Zresetuj MCU i **wyślij wielką literę `U`**.
5. Gdy zobaczysz komunikat `--- AUTO-BAUD OK! ---`, CLI jest zsynchronizowane i gotowe do pracy.

---
