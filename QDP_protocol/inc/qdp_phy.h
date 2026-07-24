#include <stdint.h>  
#include <stddef.h>  
#include <stdbool.h> 
#include <string.h>  


#ifndef  QDP_PHY_H
#define QDP_PHY_H


typedef enum Physical_states{
    PHY_STATE_WAITING,
    PHY_STATE_HEADER,
    PHY_STATE_PAYLOAD
} PHY_RX_State_t;

/**
 * @brief Function type executed by PHYafter receiving 5B header.
 * DL layer analyse header and writes to `out_payload_len` how long the
 * payload is (if garbage = 0)
 */
typedef void (*PHY_HeaderReceived_Callback_t)(const uint8_t* header_bytes, uint16_t* out_payload_len);

/**
 * @brief Function type executed by PHY after receiving full frame (Header + Payload + CRC).
 */
typedef void (*PHY_FrameReceived_Callback_t)(const uint8_t* frame_bytes, uint16_t total_len);

/**
 * @brief Pins, clocks, interupts and DMA configuration for PHY.
 * 
 * @post BSRR register force IDLE state on tx pins, and listening interupts are ready.
 */
void PHY_init(void);

/**
 * @brief Translate raw bytes into commands for BSRR register and initialize DMA.
 * 
 * @param frame_data   Ready DL layer frame pointer
 * @param frame_length Total frame length in bytes
 */
void PHY_Send(const uint8_t* frame_data, uint16_t frame_length);

/**
 * @brief Register callbacks functions for DL layer.
 * 
 * @param header_cb Function called right after receiving 5 byte header.
 * @param frame_cb  Function called right after receiving full frmae.
*/
void PHY_RegisterCallbacks(PHY_HeaderReceived_Callback_t header_cb, PHY_FrameReceived_Callback_t frame_cb);
/**
 * @brief Set RX pins into active listening mode.
 * 
 * @pre PHY_init() must be executed before.
 * @post Receiver state is set to PHY_STATE_WAITING.
 */
void PHY_Start_Listening(void);

#endif