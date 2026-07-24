#ifndef QDP_DL_H
#define QDP_DL_H

#include <stdint.h>
#include <stdbool.h>
#include "qdp_protocol.h"


typedef enum DL_tx_State {
    DL_TX_IDLE,
    DL_TX_WAIT,
    DL_TX_GO,
    DL_TX_WAIT_ACK
} DL_tx_State_t;

/**
 * @brief Analyse header and returns payload length.
 */
void DL_OnHandlerReceived(uint8_t* header_bytes, uint16_t* out_payload_len);

/**
 * @brief Receive full frame from PHY, checks CRC and (ACK/NACK/Drop) decision.
 */
void DL_FullFrameReceived(const uint8_t* frame_bytes, uint16_t total_len);


/**
 * @brief Builds DL frame and sets flag to PHY using has_pending_tx variable.
 * @param channel   channel of comunication
 * @param qos       Quality of Service type 
 * @param payload   data max 255B
 * @param length    data length 1-255B
 */
void QDP_CreateFrame(QDP_Channel_t channel, QDP_QoS_t qos, const uint8_t* payload, uint8_t length);

/**
 * @brief Informs DL layer, PHY detected preamble and started receiving.
 * 
 * @details Function called from hardware interupt level (EXTI). 
 * is used for blocking transmiter (CSMA/CA) - sets flag for medium occupancy.
 */
void QDP_DL_Notify_RxStarted(void);


void DL_TX_Process(uint32_t delta_time_ms);

void QDP_DL_Reset(void);

void DL_Handle_ACK(uint8_t received_seq);

<<<<<<< HEAD

// DEBUG FUNCTIONS 

/**
 * @brief Returns current state of QDP State Machine
 */
uint8_t Get_QDP_State_For_Debug(void);

/**
 * @brief Returns current business state of medium
 */
uint8_t Get_medium_state(void);

/**
 * @brief Returns value of backoff_timer_ms variable
 */
uint32_t Get_backoff(void);
=======
>>>>>>> 137db1f770440e5fd6a039cf1b18f0568795f80f
#endif