#include <stdint.h>  // Dla typów uint8_t, uint16_t, uint32_t
#include <stddef.h>  // Dla słowa kluczowego NULL
#include <stdbool.h> // Dla typu bool (true/false)
#include <string.h>  // Dla funkcji memcpy (przyda się w qdp_dl.c)


#ifndef  QDP_PHY_H
#define QDP_PHY_H


typedef enum Physical_states{
    PHY_STATE_WAITING,
    PHY_STATE_HEADER,
    PHY_STATE_PAYLOAD
} PHY_RX_State_t;

/**
 * @brief Typ funkcji wywoływanej przez PHY po odebraniu 5 bajtów nagłówka.
 * warstwa DL musi przeanalizować nagłówek i wpisać do `out_payload_len` ile danych 
 * należy jeszcze pobrać (0 jeśli nagłówek to śmieci).
 */
typedef void (*PHY_HeaderReceived_Callback_t)(const uint8_t* header_bytes, uint16_t* out_payload_len);

/**
 * @brief Typ funkcji wywoływanej przez PHY po odebraniu całej ramki (Nagłówek + Payload + CRC).
 */
typedef void (*PHY_FrameReceived_Callback_t)(const uint8_t* frame_bytes, uint16_t total_len);

/**
 * @brief Konfiguruje piny, zegary, przerwania i kontrolery DMA dla warstwy fizycznej.
 * 
 * @post Rejestr BSRR wymusza stan IDLE na pinach nadawczych, a przerwania nasłuchowe są gotowe.
 */
void PHY_init(void);

/**
 * @brief Przekształca surowe bajty ramki na komendy dla rejestru BSRR i inicjalizuje DMA.
 * 
 * @param frame_data   Wskaźnik na gotową ramkę z warstwy DL
 * @param frame_length Całkowita długość ramki w bajtach
 */
void PHY_Send(const uint8_t* frame_data, uint16_t frame_length);

/**
 * @brief Rejestruje funkcje zwrotne (callbacks) dla warstwy DL.
 * 
 * @param header_cb Funkcja wywoływana natychmiast po odebraniu 5-bajtowego nagłówka.
 * @param frame_cb  Funkcja wywoływana po odebraniu całej ramki z payloadem.
*/
void PHY_RegisterCallbacks(PHY_HeaderReceived_Callback_t header_cb, PHY_FrameReceived_Callback_t frame_cb);
/**
 * @brief Przełącza piny RX w tryb aktywnego nasłuchiwania na ramkę.
 * 
 * @pre Funkcja PHY_init() musi zostać wcześniej wykonana.
 * @post Zmienna stanu odbiornika zostaje ustawiona na PHY_STATE_WAITING.
 */
void PHY_Start_Listening(void);

#endif