#include "qdp_dl.h"
#include "qdp_phy.h"
#include "qdp_protocol.h"


#define DL_MAX_RETIRES 3
#define DL_TIMEOUT_MS  10

static uint8_t tx_pending_frame[QDP_MAX_FRAME_BYTES];
static uint16_t tx_pending_len = 0;
static volatile bool has_pending_tx = false;

// retransition and ACK/NACK flags
static volatile bool is_waiting_for_ack = false;
static volatile uint32_t ack_timeout_timer = 0;
static volatile uint8_t retry_counter = 0;

// Zmienna globalna dla warstwy DL - określa stan kabla
static volatile bool is_medium_busy = false;

static DL_tx_State_t dl_tx_state = DL_TX_IDLE;
static uint32_t backoff_timer_ms = 0;

static uint8_t get_random_number(void) {
    static uint32_t seed = 12345;
    seed = (seed * 1103515245 + 12345) & 0x7fffffff;
    
    // Zwraca wartość od 1 do 10 ms
    return (seed % 10) + 1;
}

void QDP_DL_Process(uint32_t delta_time_ms){
    switch (dl_tx_state)
    {
    case DL_TX_IDLE:
        if (has_pending_tx) {
            dl_tx_state = DL_TX_WAIT;
            retry_counter = 0;
        }
        break;
    case DL_TX_WAIT:
        if (!is_medium_busy) {
            backoff_timer_ms = get_random_number();
            dl_tx_state = DL_TX_GO;
        }
        break;
    case DL_TX_GO:
        if (is_medium_busy) {
            dl_tx_state = DL_TX_WAIT;
        } else {
            if (backoff_timer_ms <= delta_time_ms) {
                    backoff_timer_ms = 0;

                    PHY_Send(tx_pending_frame, tx_pending_len);

                    QDP_Header_t* frame_header = (QDP_Header_t*) tx_pending_frame;
                    
                    if (frame_header->qos == QDP_QoS_1_REQ) {
                        backoff_timer_ms = 10; // in future DL_TIMEOUT_MS
                        dl_tx_state = DL_TX_WAIT_ACK;
                    } else {
                        has_pending_tx = false;
                        dl_tx_state = DL_TX_IDLE;
                    }
            } else {
                backoff_timer_ms -= delta_time_ms;
            }
        }
        break;
    case DL_TX_WAIT_ACK:
            if (backoff_timer_ms <= delta_time_ms){ // in future DL_TIMEOUT_MS
                if (retry_counter < DL_MAX_RETIRES) {
                    retry_counter++; 
                    dl_tx_state = DL_TX_WAIT;
                } else {
                    has_pending_tx = false;
                    dl_tx_state = DL_TX_IDLE;
                }
            } else {
                    backoff_timer_ms -= delta_time_ms;
            }
        break;
    }
}

void DL_Handle_ACK(uint8_t received_seq){
    if (dl_tx_state == DL_TX_WAIT_ACK) {
        QDP_Header_t* frame_header = (QDP_Header_t*)tx_pending_frame;
        
        if (frame_header->seq == received_seq)
        {
            has_pending_tx = false;
            dl_tx_state = DL_TX_IDLE;
        }
        
    }
    
}

void QDP_DL_Notify_RxStarted(void) {
    // PHY właśnie dało znać, że coś leci po kablu!
    is_medium_busy = true; 
    
    // Złota zasada inżynierii: Jeśli czekaliśmy na ACK, a ktoś zaczął 
    // właśnie nadawać, wstrzymujemy odliczanie timeoutu na ułamek sekundy,
    // bo to COŚ co leci, to najprawdopodobniej właśnie nasze ACK!
}


void DL_OnHandlerReceived(uint8_t* header_bytes, uint16_t* out_payload_len){
    QDP_Header_t* header = (QDP_Header_t*) header_bytes;

    if (header->preamble == QDP_PREABLE_BYTE && header->magic_word == QDP_MAGIC_WORD){
        *out_payload_len = header->len;
    } else {
        *out_payload_len = 0;
        is_medium_busy = false;
    }
}

void DL_FullFrameReceived(const uint8_t* frame_bytes, uint16_t total_len){
    QDP_Header_t* header = (QDP_Header_t*)frame_bytes;
    // Checks if it is even our frame!
    if (header->preamble != QDP_PREABLE_BYTE || header->magic_word != QDP_MAGIC_WORD) {
        return; // if not dont care
    }
    uint16_t data_len = total_len - QDP_CRC_SIZE_BYTES;
    uint32_t calculated_crc = DL_Calculate_CRC(frame_bytes, data_len);

    uint32_t received_crc = (frame_bytes[data_len] << 24)     |
                        (frame_bytes[data_len + 1] << 16) |
                        (frame_bytes[data_len + 2] << 8)  |
                        (frame_bytes[data_len + 3]);
    uint8_t* payload = (uint8_t*)&frame_bytes[sizeof(QDP_Header_t)];
    if (calculated_crc == received_crc){
        if(header->qos == QDP_QoS_2_RESP && header->channel == QDP_ACK_CHN){
            DL_Handle_ACK(payload[0]);
        } else {
            APP_Process_Incoming_Data(header->channel, payload, header->len);
            if (header->qos == QDP_QoS_1_REQ){
                uint8_t ack_payload[1] = {header->seq};
                QDP_CreateFrame(QDP_ACK_CHN, QDP_QoS_2_RESP, ack_payload, 1);
            }
        } 
    } else {
        // maybe some day I'll implement some NACKs
        // uint8_t nack_payload[2] = {header->seq, QDP_NACK_CRC};
        // QDP_Send(QDP_ACK_CHN, QDP_QoS_2_RESP, nack_payload, sizeof(nack_payload));
    }
    is_medium_busy = false;
    PHY_Start_Listening();
}


void QDP_CreateFrame(QDP_Channel_t channel, QDP_QoS_t qos, const uint8_t* payload, uint8_t length){

    if (has_pending_tx)
    {
        return;
    }
    
    uint16_t total_frame_size = ACL_FRAME_SIZE(length);
    // creating the header
    QDP_Header_t* header = (QDP_Header_t*)tx_pending_frame;
    header->preamble    = QDP_PREABLE_BYTE;
    header->magic_word  = QDP_MAGIC_WORD;
    header->qos         = qos;
    header->channel     = channel;
    header->seq         = qdp_get_next_seq();
    header->len         = length;
    // append the payload
    memcpy(&tx_pending_frame[sizeof(QDP_Header_t)], payload, length);
    // calculate the CRC
    uint16_t data_to_crc_len = sizeof(QDP_Header_t) + length;
    uint32_t calc_crc = DL_Calculate_CRC(tx_pending_frame, data_to_crc_len);

    // append the CRC
    tx_pending_frame[total_frame_size - 4] = (calc_crc >> 24) & 0xFF;
    tx_pending_frame[total_frame_size - 3] = (calc_crc >> 16 & 0xFF);
    tx_pending_frame[total_frame_size - 2] = (calc_crc >> 8) & 0xFF;
    tx_pending_frame[total_frame_size - 1] = calc_crc & 0xFF;

    tx_pending_len = total_frame_size;
    has_pending_tx = true;   
}

// tests only

void QDP_DL_Reset(void) {
    has_pending_tx = false;
    dl_tx_state = DL_TX_IDLE; 
    backoff_timer_ms = 0;
    retry_counter = 0;
}

uint8_t Get_QDP_State_For_Debug(void) {
    return dl_tx_state;
}
uint8_t Get_medium_state(void) {
    return is_medium_busy;
}

uint32_t Get_backoff(void) {
    return ;
}