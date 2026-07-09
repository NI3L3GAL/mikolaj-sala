#ifndef QDP_PROTOCOL_H
#define QDP_PROTOCOL_H  

#define QDP_PREABLE_BYTE  0xCC

#define QDP_MAGIC_WORD    0xA1

#define QDP_MAX_PAYLOAD   255

#define QDP_CRC_SIZE_BYTES 4

#define QDP_LOGIC_STATES 4


typedef enum Quality_of_Service {  
    QDP_QoS_0_FnF = 0b00,  // Transmition without ACK
    QDP_QoS_1_REQ = 0b01,  // Transmition with ACK
    QDP_QoS_2_RESP = 0b10 // ACK/NACK response
} QDP_QoS_t;

typedef enum Channels{
    QDP_SYS_CHN = 0,
    QDP_SD_CHN = 1,
    QDP_ACK_CHN = 2
} QDP_Channel_t;

typedef enum NACK_errors{
    QDP_ERR_NONE    = 0x00,
    QDP_ERR_DEAD    = 0x01,
    QDP_ERR_UKN_CHN = 0x02,
    QDP_ERR_IWTB    = 0x03
} QDP_NACK_t;

#pragma pack(push, 1)
typedef struct header{
    uint8_t preamble;
    uint8_t magic_word;

    uint8_t qos     :2;
    uint8_t channel :6;

    uint8_t seq;
    uint8_t len;
} QDP_Header_t;
#pragma pack(pop)

#define QDP_MAX_FRAME_BYTES (sizeof(QDP_Header_t) + QDP_MAX_PAYLOAD + QDP_CRC_SIZE_BYTES)

#define ACL_FRAME_SIZE(payload_size) (sizeof(QDP_Header_t) + payload_size + QDP_CRC_SIZE_BYTES)

#endif