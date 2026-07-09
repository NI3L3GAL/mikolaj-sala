#ifndef QDP_DL_H
#define QDP_DL_H

#include <stdint.h>
#include <stdbool.h>
#include "qdp_protocol.h"


/**
 * @brief Analizuje nagłówek i zwraca oczekiwaną długość payloadu.
 */
void DL_OnHandlerReceived(uint8_t* header_bytes, uint16_t* out_payload_len);

/**
 * @brief Odbiera pełną ramkę z PHY, sprawdza CRC i podejmuje decyzję (ACK/NACK/Drop).
 */
void DL_FullFrameReceived(const uint8_t* frame_bytes, uint16_t total_len);


/**
 * @brief Buduje ramkę warstwy DL i wysyła ją przez warstwę fizyczną korzystając
 * z PHY_Send.
 * @param channel   kanał wysyłania
 * @param qos       rodzaj Quality of Service 
 * @param payload   wysyłane dane max 255B
 * @param length    długość głównej wiadomości 1-255B
 */
void QDP_Send(QDP_Channel_t channel, QDP_QoS_t qos, const uint8_t* payload, uint8_t length);

#endif