/***********************************************************
 * @file     system.c
 * @brief    function define:   common function, queue function, framing function.
 * @version  3.3.5
 * @date     2026.04.08
 * @copyright Copyright (c) tuya.inc 2024
 **********************************************************/

#include "zigbee.h"

/***********************************************************
 * Macro Definitions
 **********************************************************/


/***********************************************************
 * Typedef Definitions
 **********************************************************/


/***********************************************************
 * Variable Declarations
 **********************************************************/


/***********************************************************
 * Variable Definitions
 **********************************************************/

///< uart buffer
volatile unsigned char g_uart_rx_buf[UART_RX_BUF_LEN_LMT] = {0};
volatile unsigned char g_uart_tx_buf[UART_TX_BUF_LEN_LMT] = {0};

///< queue buffer
volatile unsigned char sg_queue_buf[UART_QUEUE_BUF_LEN_LMT] = {0};
volatile unsigned char *sg_queue_tail = (unsigned char *)sg_queue_buf;
volatile unsigned char *sg_queue_head = (unsigned char *)sg_queue_buf;

/***********************************************************
 * Function Declarations
 **********************************************************/

/*----------------------------------------------------------
 *                     queue function
 *--------------------------------------------------------*/

/**
 * @brief  check if the queue is empty
 * @param none
 * @return TRUE is empty
 */
static unsigned char __queue_is_empty(void);

/**
 * @brief  check if the queue is full
 * @param none
 * @return TRUE is full
 */
static unsigned char __queue_is_full(void);

/***********************************************************
 * Function Definitions
 **********************************************************/

/*----------------------------------------------------------
 *                     common function
 *--------------------------------------------------------*/

unsigned short get_seq_num(void)
{
    static volatile unsigned short s_seq_num = 0;

    s_seq_num++;
    if (s_seq_num > 0xfff0) {
        s_seq_num = 1;
    }

    return s_seq_num;
}

unsigned char get_u8_checksum(unsigned char *data, unsigned short data_len, unsigned char *checksum)
{
    if (NULL == data || NULL == checksum) {
        return FALSE;
    }

    unsigned char temp = 0;
    for (unsigned short i = 0; i < data_len; i++) {
        temp += data[i];
    }
    *checksum = temp;

    return TRUE;
}

unsigned long my_strlen(unsigned char *str)
{
    if (str == NULL) {
        return 0;
    }

    unsigned long len = 0;
    while (*str != '\0') {
        str++;
        len++;
    }
    return len;
}

void *my_memset(void *src, unsigned char ch, unsigned short count)
{
    if (src == NULL) {
        return NULL;
    }

    unsigned char *tmp = (unsigned char *)src;
    for (unsigned short i = 0; i < count; i++) {
        tmp[i] = ch;
    }
    return src;
}

void *my_memcpy(void *dest, const void *src, unsigned short count)
{
    if (dest == NULL || src == NULL) {
        return NULL;
    }

    unsigned short i = 0;
    unsigned char *temp_dest = (unsigned char *)dest;
    const unsigned char *temp_src = (const unsigned char *)src;

    if ((temp_dest <= temp_src) || (temp_dest > temp_src + count)) {
        for (i = 0; i < count; i++) {
            temp_dest[i] = temp_src[i];
        }
    } else {
        for (i = count; i > 0; i--) {
            temp_dest[i - 1] = temp_src[i - 1];
        }
    }
    return dest;
}

char *my_strcpy(char *dest, const char *src)
{
    if (dest == NULL || src == NULL) {
        return NULL;
    }

    char *temp_dest = dest;
    while (*src != '\0') {
        *dest = *src;
        dest++;
        src++;
    }
    *dest = '\0';
    return temp_dest;
}

char my_strcmp(char *s1, char *s2)
{
    if (s1 == NULL || s2 == NULL) {
        return (char)2;                   // error
    }
    
    while (1) {
        if (*s1 == '\0' && *s2 == '\0') {
            return 0;               // equal
        } else if (*s1 == '\0' || *s2 == '\0') {
            return (char)-1;              // not equal
        } else if (*s1 != *s2) {
            return (char)-1;              // not equal
        } else {
            s1++;
            s2++;
        }
    }
}

char my_strncmp(unsigned char *str1, unsigned char *str2, unsigned char len)
{
    if (NULL == str1 || NULL == str2) {
        return (char)2;                   // error
    }

    for (unsigned short i = 0; i < len; i++) {
        if (str1[i] < str2[i]) {
            return (char)-1;              // not equal
        } else if (str1[i] > str2[i]) {
            return (char)1;               // not equal
        }
    }
    return 0;                       // equal
}

unsigned char hex_to_bcd(unsigned char Value_H, unsigned char Value_L)
{
    unsigned char bcd_value = 0;

    if ((Value_H >= '0') && (Value_H <= '9')) {
        Value_H -= '0';
    } else if ((Value_H >= 'A') && (Value_H <= 'F')) {
        Value_H = Value_H - 'A' + 10;
    } else if ((Value_H >= 'a') && (Value_H <= 'f')) {
        Value_H = Value_H - 'a' + 10;
    }
    bcd_value = Value_H & 0x0f;
    bcd_value <<= 4;

    if ((Value_L >= '0') && (Value_L <= '9')) {
        Value_L -= '0';
    } else if ((Value_L >= 'A') && (Value_L <= 'F')) {
        Value_L = Value_L - 'a' + 10;
    } else if ((Value_L >= 'a') && (Value_L <= 'f')) {
        Value_L = Value_L - 'a' + 10;
    }
    bcd_value |= Value_L & 0x0f;

    return bcd_value;
}

void int_to_byte(unsigned long number, unsigned char *array)
{
    if (NULL == array) {
        return;
    }
    
    array[0] = number >> 24;
    array[1] = number >> 16;
    array[2] = number >> 8;
    array[3] = number;
}

unsigned long byte_to_int(const unsigned char *array)
{
    if (NULL == array) {
        return 0;
    }

    unsigned long nubmer = 0;
    for (unsigned char i = 0; i < 4; i++) {
        nubmer <<= 8;
        nubmer += array[i];
    }
    return nubmer;
}

char ascii_to_hex(unsigned char ascii_data)
{
    if (ascii_data < '0' || ascii_data > '9') {
        return (char)-1;
    }

    return ascii_data % 0x30;
}

/*----------------------------------------------------------
 *                     queue function
 *--------------------------------------------------------*/

unsigned char queue_dequeue_byte(unsigned char *value)
{
    if (NULL == value) {
        return FALSE;
    }
    
    if (TRUE == __queue_is_empty()) {
        return FALSE;
    }

    *value = *sg_queue_head;

    sg_queue_head++;
    if (sg_queue_head >= (unsigned char *)sg_queue_buf + sizeof(sg_queue_buf)) {
        sg_queue_head = (unsigned char *)sg_queue_buf;
    }
    
    return TRUE;
}

unsigned char queue_enqueue_byte(unsigned char value)
{
    if (TRUE == __queue_is_full()) {
        PRINT_DEBUG("[ERROR]quene full\r\n");
        return FALSE;
    }

    *sg_queue_tail = value;

    sg_queue_tail++;
    if (sg_queue_tail >= (unsigned char *)sg_queue_buf + sizeof(sg_queue_buf)) {
        sg_queue_tail = (unsigned char *)sg_queue_buf;
    }

    return TRUE;
}

static unsigned char __queue_is_empty(void)
{
    if (sg_queue_head == sg_queue_tail) {
        return TRUE;
    }

    return FALSE;
}

static unsigned char __queue_is_full(void)
{
    if (1 == sg_queue_head - sg_queue_tail) {
        return TRUE;
    }

    if (sizeof(sg_queue_buf) - 1 == sg_queue_tail - sg_queue_head) {
        return TRUE;
    }

    return FALSE;
}

/*----------------------------------------------------------
 *                     framing function
 *--------------------------------------------------------*/

void uart_framing_fill_payload_byte(unsigned short *payload_len, unsigned char byte)
{
    if ((NULL == payload_len) || (UART_TX_BUF_LEN_LMT - FRAME_LEN_WITHOUT_PAYLOAD < *payload_len + 1)) {
        return;
    }

    unsigned char *obj = (unsigned char *)g_uart_tx_buf + FRAME_FIELD_PAYLOAD_START + *payload_len;
    *obj = byte;
    *payload_len += 1;
}

void uart_framing_fill_payload_buff(unsigned short *payload_len, unsigned char *buff, unsigned short buff_len)
{
    if ((NULL == payload_len) || (NULL == buff) || (0 >= buff_len) || (UART_TX_BUF_LEN_LMT - FRAME_LEN_WITHOUT_PAYLOAD < *payload_len + buff_len)) {
        return;
    }
    
    unsigned char *obj = (unsigned char *)g_uart_tx_buf + FRAME_FIELD_PAYLOAD_START + *payload_len;
    my_memcpy(obj, buff, buff_len);
    *payload_len += buff_len;
}

#if (DEVICE_TYPE == SCENE_SWITCH_DEVICE)
void uart_framing_fill_payload_add_group_id(unsigned short *payload_len)
{
    if (UART_TX_BUF_LEN_LMT - FRAME_LEN_WITHOUT_PAYLOAD < *payload_len + 2) {
        return;
    }

    ///< 1. store the original payload
    unsigned char origin_payload_len = *payload_len;
    unsigned char origin_payload[UART_TX_BUF_LEN_LMT - FRAME_LEN_WITHOUT_PAYLOAD] = {0};
    my_memcpy(origin_payload, (const unsigned char *)g_uart_tx_buf + FRAME_FIELD_PAYLOAD_START, origin_payload_len);

    ///< 2. add group id
    g_uart_tx_buf[FRAME_FIELD_PAYLOAD_START] = g_group_id_send_dp_msg >> 8;
    g_uart_tx_buf[FRAME_FIELD_PAYLOAD_START + 1] = g_group_id_send_dp_msg;

    ///< 3. add the original payload
    my_memcpy((unsigned char *)g_uart_tx_buf + FRAME_FIELD_PAYLOAD_START + 2, origin_payload, origin_payload_len);   

    *payload_len += 2;
}
#endif

unsigned char uart_framing_fill_protocol_field(unsigned char cmd_id, unsigned short payload_len, unsigned short *seq)
{
    ///< 1. hdr, ver
    g_uart_tx_buf[FRAME_FIELD_HDR_HIGH] = FRAME_VALUE_HDR_HIGH;
    g_uart_tx_buf[FRAME_FIELD_HDR_LOW] = FRAME_VALUE_HDR_LOW;
    g_uart_tx_buf[FRAME_FIELD_PROT_VER] = FRAME_VALUE_PROT_VER;

    ///< 2. seq
    unsigned short __seq = 0;
    if (NULL == seq) {
        __seq = get_seq_num();
    } else {
        __seq = *seq;
    }
    g_uart_tx_buf[FRAME_FIELD_SEQ_HIGH] = __seq >> 8;
    g_uart_tx_buf[FRAME_FIELD_SEQ_LOW] = __seq;

    ///< 3. cmd
    g_uart_tx_buf[FRAME_FIELD_CMD_ID] = cmd_id;

    ///< 4. payload_len
    g_uart_tx_buf[FRAME_FIELD_PAYLOAD_LEN_HIGH] = payload_len >> 8;
    g_uart_tx_buf[FRAME_FIELD_PAYLOAD_LEN_LOW] = payload_len;

    ///< 5. checksum
    unsigned short frame_len = payload_len + FRAME_LEN_WITHOUT_PAYLOAD;
    unsigned char checksum = 0;
    if (FALSE == get_u8_checksum((unsigned char *)g_uart_tx_buf, frame_len - 1, &checksum)) {
        return FALSE;
    }

    g_uart_tx_buf[frame_len - 1] = checksum;
    return TRUE;
}
/* -END OF FILE-  */
