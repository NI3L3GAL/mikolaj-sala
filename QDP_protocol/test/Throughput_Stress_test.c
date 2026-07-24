#include <time.h>
#include <windows.h> 
#include "unity.h"
#include "qdp_dl.h"
#include "qdp_phy.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

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


void test_Performance_Throughput_Stress(void) {
    int num_frames = 100000000;
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

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_Performance_Throughput_Stress);
    
    return UNITY_END();
}