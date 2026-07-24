#include "qdp_phy.h"
#include "qdp_protocol.h"


// -- Global definitions -- //

#define PHY_IDLE_STATE (DP_LSB_R | DP_MSB_R | DN_LSB_R | DN_MSB_R )
#define PC_TEST_ENV // <-- Tylko do testów sprzętowych


// -- Sender definitions-- //

#ifdef PC_TEST_ENV
    // ==========================================
    // VERSION FOR PC (for unity tests)
    // Some unique values to check
    // if the LUT works properly
    // ==========================================

    #define DP_LSB_R (0x01)
    #define DP_MSB_R (0x02)
    #define DP_LSB_S (0x04)
    #define DP_MSB_S (0x08)
    
    #define DN_LSB_R (0x10)
    #define DN_MSB_R (0x20)
    #define DN_LSB_S (0x40)
    #define DN_MSB_S (0x80)

#else
    // ==========================================
    // Version for STM32
    // In BSRR register lower 16bits are SET (S),
    // and higher 16bits are RESET (R).
    // For now we use PA0-3
    // ==========================================

    #define PIN_DP_LSB 0
    #define PIN_DP_MSB 1
    #define PIN_DN_LSB 2
    #define PIN_DN_MSB 3

    // SET operations - lower half of the register
    #define DP_LSB_S (1UL << PIN_DP_LSB)
    #define DP_MSB_S (1UL << PIN_DP_MSB)
    #define DN_LSB_S (1UL << PIN_DN_LSB)
    #define DN_MSB_S (1UL << PIN_DN_MSB)

    // RESET operations - higher half of the register
    #define DP_LSB_R (1UL << (PIN_DP_LSB + 16))
    #define DP_MSB_R (1UL << (PIN_DP_MSB + 16))
    #define DN_LSB_R (1UL << (PIN_DN_LSB + 16))
    #define DN_MSB_R (1UL << (PIN_DN_MSB + 16))
#endif

// -- Receiver definitions -- //

extern volatile PHY_RX_State_t rx_state;
uint16_t remaining_bytes_to_read = 0;

static PHY_HeaderReceived_Callback_t dl_header_cb   = NULL;
static PHY_FrameReceived_Callback_t  dl_frame_cb    = NULL;

// -- Sender physical logic -- //

static const uint32_t QDP_TX_LUT[QDP_LOGIC_STATES] = {
    [0] = (DP_LSB_R | DP_MSB_R | DN_LSB_S | DN_MSB_S ), // 00 dp = 0V dm = -3V3
    [1] = (DP_LSB_S | DP_MSB_R | DN_LSB_R | DN_MSB_S ), // 01 dp = 1V1 dm = -2V2
    [3] = (DP_LSB_R | DP_MSB_S | DN_LSB_S | DN_MSB_R ), // 11 dp = 3V3 dm = 0V
    [2] = (DP_LSB_S | DP_MSB_S | DN_LSB_R | DN_MSB_R ), // 10 dp = 2V2 dm = -1V1
};
// Header(5) + Payload(255) + CRC(4) = 264 bajty * 4 = 1056 

static uint32_t phy_tx_buffer[QDP_MAX_FRAME_BYTES*QDP_LOGIC_STATES];
static uint8_t phy_rx_buffer[QDP_MAX_FRAME_BYTES*QDP_LOGIC_STATES];

void PHY_Send(const uint8_t* frame_data, uint16_t frame_length){
    uint32_t dma_idx = 0;
    for (uint16_t i = 0 ; i < frame_length; i++){
        uint8_t byte = frame_data[i];
        uint8_t nibb3 = ( byte >> 6) & 0x03;
        uint8_t nibb2 = ( byte >> 4) & 0x03;
        uint8_t nibb1 = ( byte >> 2) & 0x03;
        uint8_t nibb0 = ( byte >> 0) & 0x03;

        phy_tx_buffer[dma_idx++] = QDP_TX_LUT[nibb3];
        phy_tx_buffer[dma_idx++] = QDP_TX_LUT[nibb2];
        phy_tx_buffer[dma_idx++] = QDP_TX_LUT[nibb1];
        phy_tx_buffer[dma_idx++] = QDP_TX_LUT[nibb0];
    }
    #ifndef PC_TEST_ENV
    // DMA sending
    LL_DMA_SetDataLength( , , );
    
    LL_DMA_EnableStream( , );
    
    LL_TIM_EnableCounter();
    #endif
}


#ifndef PC_TEST_ENV
/**
 * @brief D+ interupt is triggered on first slope of preamble, for clock synchronisation
 * @warning It is hardware function (ISR).Do not call in aplication code!
 */
void EXTI0_IRQHandler(void) {
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_0)) {
        
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_0);
        
        LL_EXTI_DisableIT_0_31(LL_EXTI_LINE_0);
        
        // 3. (Phase Alignment)
        uint32_t half_period = LL_TIM_GetAutoReload(TIM3) / 2;
        LL_TIM_SetCounter(TIM3, half_period);
    
        LL_TIM_EnableCounter(TIM3);
        
        QDP_DL_Notify_RxStarted();
    }
}
#endif

// -- Receiver physical logic -- //

void PHY_RegisterCallbacks(PHY_HeaderReceived_Callback_t header_cb, PHY_FrameReceived_Callback_t frame_cb) {
    dl_header_cb = header_cb;
    dl_frame_cb  = frame_cb;
}

static inline uint8_t PHY_DEM(uint16_t adc_value){
    // set ADC bit resolution
    int adc_res = 12;
    int max = (1 << adc_res) - 1;
    if (adc_value < (max/2)/(QDP_LOGIC_STATES - 1)) return 0;
    if (adc_value < (3*max/2)/(QDP_LOGIC_STATES - 1)) return 1;
    if (adc_value < (5*max/2)/(QDP_LOGIC_STATES - 1)) return 3;
    return 2;
}

void PHY_Receive(uint8_t* out_buffer, uint16_t bytes_num){
    uint32_t adc_idx = 0;
    for(uint16_t i = 0; i < bytes_num; i++){
        uint8_t nibb3 = PHY_DEM(phy_rx_buffer[adc_idx++]);
        uint8_t nibb2 = PHY_DEM(phy_rx_buffer[adc_idx++]);
        uint8_t nibb1 = PHY_DEM(phy_rx_buffer[adc_idx++]);
        uint8_t nibb0 = PHY_DEM(phy_rx_buffer[adc_idx++]);

        out_buffer[i] = (nibb3 << 6) | (nibb2 << 4) | (nibb1 << 2) | nibb0;
    } 
}
 
/**
 * @brief Main handler of DMA datastream interupt for receiver.
 * 
 * @warning It is hardware function (ISR).Do not call in aplication code!
 */
void DMA2_Stream1_IRQHandler(void){
#ifndef PC_TEST_ENV
    if (LL_DMA_IsActiveFlag_TC1()){
        LL_DMA_Clear_Flag_TC1();

        LL_TIM_DisableCounter();
        LL_DMA_DisableStream( , );
#else
    // Durring tests we assume the DMA finished the work 
    if (true) {
#endif
// -----------------------------------------------------

        if (rx_state == PHY_STATE_HEADER){
            uint8_t header_buff[sizeof(QDP_Header_t)];
            PHY_Receive(header_buff, 5);

            remaining_bytes_to_read = 0;
            if (dl_header_cb != NULL) {
                dl_header_cb(header_buff, &remaining_bytes_to_read);
            }

            if (remaining_bytes_to_read > 0 ){
                uint16_t bytes_to_fetch = remaining_bytes_to_read + QDP_CRC_SIZE_BYTES;
                
// -----------------------------------------------------
// HARDWARE BLOCK: Another frame for DMA to handle
// -----------------------------------------------------
#ifndef PC_TEST_ENV
                LL_DMA_SetDataLength( , , bytes_to_fetch * 4);
                LL_DMA_EnableStream( , );
                LL_TIM_EnableCounter();
#endif
// -----------------------------------------------------
                
                rx_state = PHY_STATE_PAYLOAD;
            } else {
                PHY_Start_Listening();
            }
        } else if (rx_state == PHY_STATE_PAYLOAD){
            uint16_t total_len = 5 + remaining_bytes_to_read + QDP_CRC_SIZE_BYTES;
            uint8_t frame_buff[QDP_MAX_FRAME_BYTES];

            PHY_Receive(frame_buff, total_len);

            if (dl_frame_cb != NULL){
                dl_frame_cb(frame_buff, total_len);
            } else {
                PHY_Start_Listening();
            }
            
        } 

    }
    
}

void PHY_Start_Listening(void){
    rx_state = PHY_STATE_WAITING;
#ifndef PC_TEST_ENV
    LL_EXTI_ClearFlag_0_31();
    LL_EXTI_EnableIT_0_31();
#endif
}

// -- Globa physical logic -- //

void PHY_init(void){
#ifndef PC_TEST_ENV
    LL_DMA_ConfigAddresses();

    LL_TIM_EnableDMAReq_UPDATE();

    LL_DMA_EnableIT_TC();

    GPIOx->BSRR = PHY_IDLE_STATE;
#endif
}
