#include "unity.h"
#include "qdp_dl.h"
#include "qdp_phy.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

// ---------------------------------------------------------
// MOCKI I FLAGI (do weryfikacji co się dzieje pod maską)
// ---------------------------------------------------------

// Podgląd tego, co MAC wypluwa na druty (PHY):
uint8_t mock_ostatnia_ramka[QDP_MAX_FRAME_BYTES];
uint16_t mock_ostatnia_dlugosc = 0;

static uint8_t mock_app_process_calls = 0;
static int mock_phy_send_calls = 0;

// Podgląd tego, co MAC pcha w górę (do warstwy aplikacji):
bool spy_app_called = false;
uint8_t spy_app_channel = 0;
uint8_t spy_app_payload[256];

// Flaga od ACK
bool spy_ack_called = false;


// ---------------------------------------------------------
// ZAŚLEPKI FUNKCJI Z ZEWNĄTRZ (zastępują prawdziwy sprzęt)
// ---------------------------------------------------------

void PHY_Send(const uint8_t* frame_data, uint16_t frame_length) {
    memcpy(mock_ostatnia_ramka, frame_data, frame_length);
    mock_ostatnia_dlugosc = frame_length;
    mock_phy_send_calls++;
}

void PHY_Start_Listening(void) {}

// Sztuczne CRC, żeby nie musieć liczyć prawdziwych sum w każdym teście.
// Zawsze zwraca 0xDEADBEEF, więc łatwo to zweryfikować.
uint32_t MAC_Calculate_CRC(const uint8_t* data, uint16_t length) {
    return 0xDEADBEEF; 
}

uint32_t DL_Calculate_CRC(const uint8_t* data, uint16_t length) {
    return 0xDEADBEEF; 
}

// Numery sekwencji lecą na sztywno
uint8_t qdp_get_next_seq(void) { return 1; }

// Przechwytujemy dane lecące do apki
void APP_Process_Incoming_Data(uint8_t channel, const uint8_t* payload, uint8_t len) {
    spy_app_called = true;
    spy_app_channel = channel;
    memcpy(spy_app_payload, payload, len);
    mock_app_process_calls++;
}


// ---------------------------------------------------------
// SETUP & TEARDOWN
// ---------------------------------------------------------
void setUp(void) {
    // Twardy reset przed każdym testem, żeby flagi z poprzedniego nam nie zepsuły wyniku
    memset(mock_ostatnia_ramka, 0, sizeof(mock_ostatnia_ramka));
    mock_ostatnia_dlugosc = 0;
    mock_phy_send_calls = 0;
    
    spy_app_called = false;
    spy_app_channel = 0;
    memset(spy_app_payload, 0, sizeof(spy_app_payload));
    spy_ack_called = false;

    // Reset samej maszyny stanów
    QDP_DL_Reset();

    printf("\n--- Start Testu ---\n");
}

void tearDown(void) {}


// ---------------------------------------------------------
// TESTY 
// ---------------------------------------------------------

// Podstawowe wysyłanie - czy dobrze skleja nagłówek i wypycha ramkę
void test_QDP_Send_Powinno_Poprawnie_Zbudowac_Naglowek(void) {
    bool has_pending_tx = false; 
    uint8_t test_payload[] = {0xDE, 0xAD, 0xBE, 0xEF};
    uint8_t expected_total_len = 5 + 4 + 4; 
    
    QDP_CreateFrame(1, 1, test_payload, 4);

    // Tyknięcia maszyny: IDLE -> WAIT -> GO -> strzał
    QDP_DL_Process(1);
    QDP_DL_Process(1);
    QDP_DL_Process(10);
    
    TEST_ASSERT_EQUAL_UINT16(expected_total_len, mock_ostatnia_dlugosc);
    TEST_ASSERT_EQUAL_UINT8(4, mock_ostatnia_ramka[4]); // pole len
    TEST_ASSERT_EQUAL_HEX8(0xDE, mock_ostatnia_ramka[5]); // pierwszy bajt payloadu
}

// Dobra ramka z kabla -> powinna polecieć do apki
void test_DL_FullFrameReceived_Powinno_Przekazac_Do_Aplikacji_Gdy_CRC_Jest_Dobre(void) {
    uint8_t incoming_frame[13] = {0};
    
    // Fejkujemy nagłówek
    QDP_Header_t* hdr = (QDP_Header_t*)incoming_frame;
    hdr->preamble = 0xCC; 
    hdr->magic_word = 0xA1;
    hdr->qos = 0;      // Bez ACK
    hdr->channel = 5; 
    hdr->len = 4;      
    
    incoming_frame[5] = 0x11;
    incoming_frame[6] = 0x22;
    incoming_frame[7] = 0x33;
    incoming_frame[8] = 0x44;
    
    // Na końcu wbijamy nasze testowe CRC (0xDEADBEEF)
    incoming_frame[9]  = 0xDE;
    incoming_frame[10] = 0xAD;
    incoming_frame[11] = 0xBE;
    incoming_frame[12] = 0xEF;
    
    // Odbiór...
    DL_FullFrameReceived(incoming_frame, 13);
    
    // Apka musiała to odebrać, ACK nie miało prawa wystrzelić
    TEST_ASSERT_TRUE_MESSAGE(spy_app_called, "Funkcja APP_Process_Incoming_Data NIE zostala wywolana!");
    TEST_ASSERT_EQUAL_UINT8(5, spy_app_channel);
    TEST_ASSERT_EQUAL_HEX8(0x11, spy_app_payload[0]);
    TEST_ASSERT_FALSE_MESSAGE(spy_ack_called, "ACK zostalo wywolane, a nie powinno dla QoS 0!");
}

// Złe CRC -> dropujemy w cholerę
void test_DL_FullFrameReceived_Powinno_Odrzucic_Ramke_Ze_Zlym_CRC(void) {
    uint8_t bad_frame[13] = {0};
    bad_frame[4] = 4; // len
    
    // Śmieciowe CRC (nie pasuje do 0xDEADBEEF)
    bad_frame[9]  = 0xFF;
    bad_frame[10] = 0xFF;
    bad_frame[11] = 0xFF;
    bad_frame[12] = 0xFF;
    
    DL_FullFrameReceived(bad_frame, 13);
    
    TEST_ASSERT_FALSE_MESSAGE(spy_app_called, "Aplikacja odebrala uszkodzona ramke!");
}

// PHY wyciąga dobrą długość z nagłówka
void test_DL_OnHandlerReceived_Powinno_Zwrocic_Dlugosc_Dla_Dobrego_Naglowka(void) {
    uint8_t dobry_naglowek[5] = {0xCC, 0xA1, 0x00, 0x05, 0x0A}; // len = 10
    uint16_t odczytana_dlugosc = 99; // wartość początkowa żeby sprawdzić czy funkcja to nadpisze

    DL_OnHandlerReceived(dobry_naglowek, &odczytana_dlugosc);

    TEST_ASSERT_EQUAL_UINT16(10, odczytana_dlugosc);
}

// Błędny nagłówek na starcie -> PHY powinno zignorować i zwrócić 0
void test_DL_OnHandlerReceived_Powinno_Zwrocic_Zero_Dla_Smietnika(void) {
    uint8_t zly_naglowek[5] = {0xFF, 0xFF, 0x00, 0x05, 0x0A}; // zła preambuła
    uint16_t odczytana_dlugosc = 99; 

    DL_OnHandlerReceived(zly_naglowek, &odczytana_dlugosc);

    TEST_ASSERT_EQUAL_UINT16(0, odczytana_dlugosc);
}

// Czy działa unikanie kolizji (czekanie na backoff)
void test_QDP_Send_Powinno_Oczekiwac_Na_Backoff(void) {
    uint8_t payload[] = {0xAA};

    // Wrzucone do kolejki, ale fizycznie nie leci
    QDP_CreateFrame(1, 0, payload, 1);
    TEST_ASSERT_EQUAL_UINT16(0, mock_ostatnia_dlugosc);

    // Maszyna zauważa ramkę
    QDP_DL_Process(1);
    TEST_ASSERT_EQUAL_UINT16(0, mock_ostatnia_dlugosc);

    // Losuje backoff i czeka
    QDP_DL_Process(1);
    TEST_ASSERT_EQUAL_UINT16(0, mock_ostatnia_dlugosc);

    // Przewijamy czas o 10ms do przodu -> teraz musi pójść
    QDP_DL_Process(10); 
    TEST_ASSERT_TRUE_MESSAGE(mock_ostatnia_dlugosc > 0, "Ramka utknela w buforze mimo uplywu czasu!");
}

// Zajęte medium -> wstrzymujemy wysyłanie
void test_QDP_Send_Nie_Powinno_Wysylac_Gdy_Kabel_Zajety(void) {
    uint8_t payload[] = {0xBB};

    // Udajemy że coś leci po kablu
    QDP_DL_Notify_RxStarted();

    // Próbujemy nadawać
    QDP_CreateFrame(1, 0, payload, 1);

    // Przewijamy czas o 100ms - układ dalej musi czekać w blokadzie
    QDP_DL_Process(50);
    QDP_DL_Process(50);
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, mock_ostatnia_dlugosc, "Wyslal ramke mimo zajetego kabla!");

    // Zwalniamy kabel. Hack: karmimy MACa złym nagłówkiem, to go zresetuje do trybu słuchania.
    uint8_t zly_naglowek[5] = {0xFF, 0xFF, 0, 0, 0};
    uint16_t dummy_len;
    DL_OnHandlerReceived(zly_naglowek, &dummy_len);

    // Kabel wolny, idzie backoff
    QDP_DL_Process(1);
    QDP_DL_Process(10); // Czas mija

    TEST_ASSERT_TRUE_MESSAGE(mock_ostatnia_dlugosc > 0, "Nie wyslano ramki po zwolnieniu kabla!");
}

// Atak na pole Len (Framing Error) - sprawdzamy czy apka dostanie śmieci
void test_DL_Powinno_Odrzucic_Ramke_Z_Oszukanym_Len_Przez_Bledne_CRC(void) {
    mock_app_process_calls = 0;
    uint8_t ramka_testowa[QDP_MAX_FRAME_BYTES] = {0};
    
    // Budujemy w 100% poprawną ramkę (2 bajty danych)
    QDP_Header_t* header = (QDP_Header_t*)ramka_testowa;
    header->preamble = QDP_PREABLE_BYTE; 
    header->magic_word = QDP_MAGIC_WORD; 
    header->qos = QDP_QoS_0_FnF;
    header->channel = 1;
    header->seq = 1;
    header->len = 2; // Prawdziwy rozmiar
    
    ramka_testowa[5] = 0xAA; 
    ramka_testowa[6] = 0xBB; 
    
    // Dopisujemy prawidłowe CRC dla całości
    uint32_t poprawne_crc = DL_Calculate_CRC(ramka_testowa, 7);
    ramka_testowa[7] = (poprawne_crc >> 24) & 0xFF;
    ramka_testowa[8] = (poprawne_crc >> 16) & 0xFF;
    ramka_testowa[9] = (poprawne_crc >> 8)  & 0xFF;
    ramka_testowa[10] = poprawne_crc & 0xFF;

    // A teraz hakujemy nagłówek symulując błąd transmisji na kablu
    header->len = 100;

    // KROK 1: PHY powinno uwierzyć i chcieć ssać 100 bajtów
    uint16_t odczytana_dlugosc = 0;
    DL_OnHandlerReceived(ramka_testowa, &odczytana_dlugosc);
    TEST_ASSERT_EQUAL_UINT16(100, odczytana_dlugosc);

    // KROK 2: Wrzucamy do MACa tę zepsutą ramkę powiększoną o zerowe bajty (pobierane z ciszy na kablu)
    // 5 (hdr) + 100 (zepsute len) + 4 (crc) = 109
    DL_FullFrameReceived(ramka_testowa, 109);

    // Ostateczny test: CRC poleciało z gruzem, więc pakiet musi być odrzucony
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, mock_app_process_calls, "Krytyczny blad: Przepuszczono wadliwa ramke do aplikacji!");
}

// Odbiór ACK na czas - brak retransmisji
void test_DL_Powinno_Zakonczyc_Transmisje_Po_Otrzymaniu_ACK(void) {
    uint8_t payload[] = {0x11};
    
    // Strzał z wymogiem ACK
    QDP_CreateFrame(1, QDP_QoS_1_REQ, payload, 1);
    
    QDP_DL_Process(1);  
    QDP_DL_Process(1);  
    QDP_DL_Process(10); 
    
    TEST_ASSERT_EQUAL_UINT8(1, mock_phy_send_calls);

    // Udajemy odpowiedź z idealnym ACK
    uint8_t ack_frame[9] = {0}; 
    QDP_Header_t* ack_hdr = (QDP_Header_t*)ack_frame;
    ack_hdr->preamble = 0xCC;
    ack_hdr->magic_word = 0xA1;
    ack_hdr->qos = QDP_QoS_2_RESP;
    ack_hdr->channel = QDP_ACK_CHN; 
    ack_hdr->seq = 1; // Musi być to samo co wysłaliśmy!
    ack_hdr->len = 0; 
    
    uint32_t crc = DL_Calculate_CRC(ack_frame, 5);
    ack_frame[5] = (crc >> 24) & 0xFF; ack_frame[6] = (crc >> 16) & 0xFF;
    ack_frame[7] = (crc >> 8) & 0xFF;  ack_frame[8] = crc & 0xFF;
    
    // Wpuszczamy ACK
    DL_FullFrameReceived(ack_frame, 9);
    
    // Przewijamy czas (powyżej timeoutu)
    QDP_DL_Process(50);
    
    // Skoro weszło ACK, maszyna miała wrócić do IDLE - licznik wywołań PHY nie rośnie
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, mock_phy_send_calls, " Wyslano retransmisje mimo odebrania ACK!");
}

// Timeout ACK i wymuszona retransmisja
void test_DL_Powinno_Zrobic_Retransmisje_Gdy_Brak_ACK(void) {
    uint8_t payload[] = {0x22};
    
    // Pierwsza wysyłka
    QDP_CreateFrame(1, QDP_QoS_1_REQ, payload, 1);
    QDP_DL_Process(1);
    QDP_DL_Process(1);
    QDP_DL_Process(10);
    
    TEST_ASSERT_EQUAL_UINT8(1, mock_phy_send_calls);
    
    // Upływa czas na odpowiedź, nikt nie odpisuje
    QDP_DL_Process(11); 
    
    // Maszyna powinna wejść w retransmisję (nowy backoff i strzał)
    QDP_DL_Process(1);  
    QDP_DL_Process(10); 
    
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2, mock_phy_send_calls, " Maszyna nie zrobila retransmisji po timeoucie!");
}

// Kapitulacja po przekroczeniu Max Retries
void test_DL_Powinno_Porzucic_Ramke_Po_Przekroczeniu_Limitu(void) {
    uint8_t payload[] = {0x33};
    QDP_CreateFrame(1, QDP_QoS_1_REQ, payload, 1);
    
    // Katujemy maszynę nieskończonymi timeoutami (zakładamy limit np. 3 retransmisji)
    for (int i = 0; i < 10; i++) {
        QDP_DL_Process(1);  // WAIT -> GO
        QDP_DL_Process(10); // Strzał
        QDP_DL_Process(15); // Timeout
    }
    
    // 1 oryginał + 3 powtórki = 4 wywołania PHY (jeśli DL_MAX_RETIRES to 3)
    // Zmienić cyfrę w asercji jeśli limit w kodzie jest inny!
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(4, mock_phy_send_calls, " Maszyna nie poddala sie po przekroczeniu DL_MAX_RETIRES!");
}
#include <time.h>
#include <windows.h> 

void test_Performance_Throughput_Stress(void) {
    int num_frames = 100000;
    uint8_t payload[1024]; // Testujemy V1
    
    clock_t start = clock();
    
    for (int i = 0; i < num_frames; i++) {
        QDP_CreateFrame(1, QDP_QoS_0_FnF, payload, 1024);
        int safety_counter = 0;
        // Zmuszamy maszynę do wysyłki
        while (mock_phy_send_calls <= i) {
            QDP_DL_Process(1); // Tu maszyna przelicza logikę

            safety_counter++;
            if (safety_counter > 1000) {
                printf("DEBUG: DEADLOCK przy ramce %d!\n", i);
                return; 
            }
        }
    }
    
    clock_t end = clock();

    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    
    // Zabezpieczenie przed dzieleniem przez zero
    if (time_spent < 0.0001) time_spent = 0.0001; 
    
    double total_frames = (double)mock_phy_send_calls; 
    double total_bytes = total_frames * 1024;

    // Debug: Sprawdźmy co mamy
    printf("DEBUG: Liczba ramek: %f\n", total_frames);
    printf("DEBUG: Czas: %f\n", time_spent);

    // Obliczenie
    double throughput = (total_bytes / 1024.0 / 1024.0) / time_spent; 

    printf("\n--- WYNIK KONCOWY ---\n");
    printf("Przeslano: %.2f KB\n", total_bytes / 1024.0);
    printf("Srednia przepustowosc: %.2f MB/s\n", throughput);
}

// ---------------------------------------------------------
// ODPALAMY TESTY
// ---------------------------------------------------------
int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_QDP_Send_Powinno_Poprawnie_Zbudowac_Naglowek);
    RUN_TEST(test_DL_FullFrameReceived_Powinno_Przekazac_Do_Aplikacji_Gdy_CRC_Jest_Dobre);
    RUN_TEST(test_DL_FullFrameReceived_Powinno_Odrzucic_Ramke_Ze_Zlym_CRC);
    RUN_TEST(test_DL_Powinno_Odrzucic_Ramke_Z_Oszukanym_Len_Przez_Bledne_CRC);
    RUN_TEST(test_DL_OnHandlerReceived_Powinno_Zwrocic_Zero_Dla_Smietnika);
    RUN_TEST(test_DL_OnHandlerReceived_Powinno_Zwrocic_Dlugosc_Dla_Dobrego_Naglowka);
    RUN_TEST(test_QDP_Send_Powinno_Oczekiwac_Na_Backoff);
    RUN_TEST(test_QDP_Send_Nie_Powinno_Wysylac_Gdy_Kabel_Zajety);
    RUN_TEST(test_DL_Powinno_Zakonczyc_Transmisje_Po_Otrzymaniu_ACK);
    RUN_TEST(test_DL_Powinno_Zrobic_Retransmisje_Gdy_Brak_ACK);
    RUN_TEST(test_DL_Powinno_Porzucic_Ramke_Po_Przekroczeniu_Limitu);
    RUN_TEST(test_Performance_Throughput_Stress);
    
    return UNITY_END();
}