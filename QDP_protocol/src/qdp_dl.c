#include "qdp_dl.h"
#include "qdp_phy.h"
#include "qdp_protocol.h"


#define DL_MAX_RETIRES 3
#define DL_TIMEOUT_MS  10

static uint8_t last_tx_frame[QDP_MAX_FRAME_BYTES];
static uint16_t last_tx_len = 0;
static uint8_t last_tx_seq = 0;

static volatile bool is_waiting_for_ack = false;
static volatile uint32_t ack_timeout_timer = 0;
static volatile uint8_t retry_counter = 0;


void DL_OnHandlerReceived(uint8_t* header_bytes, uint16_t* out_payload_len){
    QDP_Header_t* header = (QDP_Header_t*) header_bytes;

    if (header->preamble == QDP_PREABLE_BYTE && header->magic_word == QDP_MAGIC_WORD){
        *out_payload_len = header->len;
    } else {
        *out_payload_len = 0;
    }
}

void DL_FullFrameReceived(const uint8_t* frame_bytes, uint16_t total_len){
    QDP_Header_t* header = (QDP_Header_t*)frame_bytes;
    // SZYBKI FILTR: Jeśli to nie jest nasza preambuła lub magic word, od razu do kosza!
    if (header->preamble != QDP_PREABLE_BYTE || header->magic_word != QDP_MAGIC_WORD) {
        return; // Ubijamy procesowanie
    }
    uint16_t data_len = total_len - QDP_CRC_SIZE_BYTES;
    uint32_t calculated_crc = DL_Calculate_CRC(frame_bytes, data_len);

    uint32_t received_crc = (frame_bytes[data_len] << 24)     |
                        (frame_bytes[data_len + 1] << 16) |
                        (frame_bytes[data_len + 2] << 8)  |
                        (frame_bytes[data_len + 3]);
    uint8_t* payload = (uint8_t*)&frame_bytes[sizeof(QDP_Header_t)];
    if (calculated_crc == received_crc){
        if(header->qos == QDP_QoS_1_REQ && header->channel == QDP_ACK_CHN){
            DL_Handle_ACK(payload[0]);
        } else {
            APP_Process_Incoming_Data(header->channel, payload, header->len);
            if (header->qos == QDP_QoS_1_REQ){
                uint8_t ack_payload[1] = {header->seq};
                QDP_Send(QDP_ACK_CHN, QDP_QoS_2_RESP, ack_payload, 1);
            }
        } 
    } else {
        // uint8_t nack_payload[2] = {header->seq, QDP_NACK_CRC};
        // QDP_Send(QDP_ACK_CHN, QDP_QoS_2_RESP, nack_payload, sizeof(nack_payload));
    }
    PHY_Start_Listening();
}


void QDP_Send(QDP_Channel_t channel, QDP_QoS_t qos, const uint8_t* payload, uint8_t length){

    uint16_t total_frame_size = ACL_FRAME_SIZE(length);
    uint8_t dl_frame_buffer[QDP_MAX_FRAME_BYTES];
    
    QDP_Header_t* header = (QDP_Header_t*)dl_frame_buffer;
    header->preamble    = QDP_PREABLE_BYTE;
    header->magic_word  = QDP_MAGIC_WORD;
    header->qos         = qos;
    header->channel     = channel;
    header->seq         = qdp_get_next_seq();
    header->len         = length;
    memcpy(&dl_frame_buffer[sizeof(QDP_Header_t)], payload, length);

    uint16_t data_to_crc_len = sizeof(QDP_Header_t) + length;
    uint32_t calc_crc = MAC_Calculate_CRC(dl_frame_buffer, data_to_crc_len);


    dl_frame_buffer[total_frame_size - 4] = (calc_crc >> 24) & 0xFF;
    dl_frame_buffer[total_frame_size - 3] = (calc_crc >> 16 & 0xFF);
    dl_frame_buffer[total_frame_size - 2] = (calc_crc >> 8) & 0xFF;
    dl_frame_buffer[total_frame_size - 1] = calc_crc & 0xFF;

    if (qos == QDP_QoS_1_REQ){
        memcpy(last_tx_frame, dl_frame_buffer, total_frame_size);
        last_tx_len = total_frame_size;
        last_tx_seq = header->seq;

        is_waiting_for_ack = true;
        ack_timeout_timer = DL_TIMEOUT_MS;
        retry_counter = 0;
    }
    PHY_Send(dl_frame_buffer, total_frame_size);
}