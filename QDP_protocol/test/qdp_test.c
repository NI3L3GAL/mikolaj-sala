#include "unity.h"
#include "qdp_dl.h"
#include "qdp_phy.h"
#include <string.h>
#include <stdbool.h>

// ==========================================
// 1. ZMIENNE SZPIEGOWSKIE (Spies)
// ==========================================
// Do sprawdzania wysyłania:

uint8_t mock_ostatnia_ramka[QDP_MAX_FRAME_BYTES];
uint16_t mock_ostatnia_dlugosc = 0;

// Do sprawdzania odbierania (czy warstwa MAC zawołała Aplikację):

bool spy_app_called = false;
uint8_t spy_app_channel = 0;
uint8_t spy_app_payload[256];

// Do sprawdzania ACK:
bool spy_ack_called = false;

// ==========================================
// 2. ZAŚLEPKI FUNKCJI (Mocki)
// ==========================================
void PHY_Send(const uint8_t* frame_data, uint16_t frame_length) {
    memcpy(mock_ostatnia_ramka, frame_data, frame_length);
    mock_ostatnia_dlugosc = frame_length;
}

void PHY_Start_Listening(void) {}

// Nasz sztuczny algorytm CRC na potrzeby testów ZAWSZE zwraca 0xDEADBEEF
uint32_t MAC_Calculate_CRC(const uint8_t* data, uint16_t length) {
    return 0xDEADBEEF; 
}

uint32_t DL_Calculate_CRC(const uint8_t* data, uint16_t length) {
    return 0xDEADBEEF; 
}

uint8_t qdp_get_next_seq(void) { return 1; }

// Gdy warstwa MAC chce przekazać dane wyżej, "szpieg" to zapisuje
void APP_Process_Incoming_Data(uint8_t channel, const uint8_t* payload, uint8_t len) {
    spy_app_called = true;
    spy_app_channel = channel;
    memcpy(spy_app_payload, payload, len);
}

void DL_Handle_ACK(uint8_t seq) {
    spy_ack_called = true;
}

// ==========================================
// 3. KONFIGURACJA TESTÓW (setUp)
// ==========================================
void setUp(void) {
    // Zerujemy szpiegów przed KAŻDYM testem!
    memset(mock_ostatnia_ramka, 0, sizeof(mock_ostatnia_ramka));
    mock_ostatnia_dlugosc = 0;
    
    spy_app_called = false;
    spy_app_channel = 0;
    memset(spy_app_payload, 0, sizeof(spy_app_payload));
    spy_ack_called = false;
}

void tearDown(void) {}

// ==========================================
// 4. TESTY JEDNOSTKOWE
// ==========================================

// TEST 1: WYSYŁANIE (Ten, który już mieliśmy)
void test_QDP_Send_Powinno_Poprawnie_Zbudowac_Naglowek(void) {
    uint8_t test_payload[] = {0xDE, 0xAD, 0xBE, 0xEF};
    uint8_t expected_total_len = 5 + 4 + 4; 
    
    QDP_Send(1, 1, test_payload, 4); 
    
    TEST_ASSERT_EQUAL_UINT16(expected_total_len, mock_ostatnia_dlugosc);
    TEST_ASSERT_EQUAL_UINT8(4, mock_ostatnia_ramka[4]); // Sprawdzenie długości (header->len)
    TEST_ASSERT_EQUAL_HEX8(0xDE, mock_ostatnia_ramka[5]); // Sprawdzenie payloadu
}

// TEST 2: ODBIERANIE I PRZEKAZYWANIE DANYCH
void test_DL_FullFrameReceived_Powinno_Przekazac_Do_Aplikacji_Gdy_CRC_Jest_Dobre(void) {
    // 1. Arrange: Symulujemy ramkę, która właśnie "przyszła z kabla"
    uint8_t incoming_frame[13] = {0};
    
    // Budujemy sztuczny nagłówek na początku naszej ramki
    QDP_Header_t* hdr = (QDP_Header_t*)incoming_frame;
    hdr->preamble = 0xCC; 
    hdr->magic_word = 0xA1;
    hdr->qos = 0;      // QoS 0 - tylko przekazujemy dane, bez ACK
    hdr->channel = 5;  // Przykładowy kanał 5
    hdr->len = 4;      // 4 bajty payloadu
    
    // Wrzucamy ładunek użyteczny
    incoming_frame[5] = 0x11;
    incoming_frame[6] = 0x22;
    incoming_frame[7] = 0x33;
    incoming_frame[8] = 0x44;
    
    // Dodajemy na sam koniec sumę kontrolną pasującą do naszej Zaślepki (0xDEADBEEF)
    incoming_frame[9]  = 0xDE;
    incoming_frame[10] = 0xAD;
    incoming_frame[11] = 0xBE;
    incoming_frame[12] = 0xEF;
    
    // 2. Act: Wstrzykujemy ramkę do Twojej logiki
    DL_FullFrameReceived(incoming_frame, 13);
    
    // 3. Assert: Weryfikujemy, czy logika zareagowała poprawnie
    TEST_ASSERT_TRUE_MESSAGE(spy_app_called, "Funkcja APP_Process_Incoming_Data NIE zostala wywolana!");
    TEST_ASSERT_EQUAL_UINT8(5, spy_app_channel);
    TEST_ASSERT_EQUAL_HEX8(0x11, spy_app_payload[0]);
    TEST_ASSERT_FALSE_MESSAGE(spy_ack_called, "ACK zostalo wywolane, a nie powinno dla QoS 0!");
}

// TEST 3: ODRZUCANIE ZŁEGO CRC
void test_DL_FullFrameReceived_Powinno_Odrzucic_Ramke_Ze_Zlym_CRC(void) {
    uint8_t bad_frame[13] = {0};
    bad_frame[4] = 4; // len = 4
    
    // Dajemy BŁĘDNE CRC na koniec (inne niż nasze 0xDEADBEEF)
    bad_frame[9]  = 0xFF;
    bad_frame[10] = 0xFF;
    bad_frame[11] = 0xFF;
    bad_frame[12] = 0xFF;
    
    DL_FullFrameReceived(bad_frame, 13);
    
    // Aplikacja absolutnie NIE MOGŁA zostać powiadomiona o tych danych
    TEST_ASSERT_FALSE_MESSAGE(spy_app_called, "Aplikacja odebrala uszkodzona ramke!");
}

void test_DL_OnHandlerReceived_Powinno_Zwrocic_Dlugosc_Dla_Dobrego_Naglowka(void) {
    uint8_t dobry_naglowek[5] = {0xCC, 0xA1, 0x00, 0x05, 0x0A}; // CC AA, QoS 0, Ch 5, Len 10
    uint16_t odczytana_dlugosc = 99; // Wrzucamy bzdurę, żeby sprawdzić czy nadpisze

    DL_OnHandlerReceived(dobry_naglowek, &odczytana_dlugosc);

    TEST_ASSERT_EQUAL_UINT16(10, odczytana_dlugosc);
}

void test_DL_OnHandlerReceived_Powinno_Zwrocic_Zero_Dla_Smietnika(void) {
    uint8_t zly_naglowek[5] = {0xFF, 0xFF, 0x00, 0x05, 0x0A}; // Zła preambuła
    uint16_t odczytana_dlugosc = 99; 

    DL_OnHandlerReceived(zly_naglowek, &odczytana_dlugosc);

    TEST_ASSERT_EQUAL_UINT16(0, odczytana_dlugosc);
}

// ==========================================
// 5. MAIN
// ==========================================
int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_QDP_Send_Powinno_Poprawnie_Zbudowac_Naglowek);
    RUN_TEST(test_DL_FullFrameReceived_Powinno_Przekazac_Do_Aplikacji_Gdy_CRC_Jest_Dobre);
    RUN_TEST(test_DL_FullFrameReceived_Powinno_Odrzucic_Ramke_Ze_Zlym_CRC);
    RUN_TEST(test_DL_OnHandlerReceived_Powinno_Zwrocic_Zero_Dla_Smietnika);
    RUN_TEST(test_DL_OnHandlerReceived_Powinno_Zwrocic_Dlugosc_Dla_Dobrego_Naglowka);
    
    return UNITY_END();
}