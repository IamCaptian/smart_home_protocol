/***********************************************************
 * @file     system.h
 * @brief    function declare:  common function, queue function, framing function.
 * @version  3.3.5
 * @date     2026.04.08
 * @copyright Copyright (c) tuya.inc 2024
 **********************************************************/

#ifndef __SYSTEM_H_
#define __SYSTEM_H_

#include "protocol.h"

#ifdef __cplusplus
extern "C"
{
#endif

/***********************************************************
 * Macro Definitions
 **********************************************************/


/***********************************************************
 * Typedef Definitions
 **********************************************************/


/***********************************************************
 * Function Declarations
 **********************************************************/

/*----------------------------------------------------------
 *                     common function
 *--------------------------------------------------------*/

/**
 * @brief  get a new seq num for uart protocol frame
 * @param none
 * @return seq num
 */
unsigned short get_seq_num(void);

/**
 * @brief  calculate the checksum (unsigned char)
 * @param[in] data
 * @param[in] data_len
 * @param[out] checksum
 * @return operate result
 */
unsigned char get_u8_checksum(unsigned char *data, unsigned short data_len, unsigned char *checksum);

/**
 * @brief  strlen
 * @param[in] str
 * @return string len
 */
unsigned long my_strlen(unsigned char *str);

/**
 * @brief  memset
 * @param[in] src
 * @param[in] ch
 * @param[in] count
 * @return src
 */
void *my_memset(void *src, unsigned char ch, unsigned short count);

/**
 * @brief  memcpy
 * @param[in] dest
 * @param[in] src
 * @param[in] count
 * @return dest
 */
void *my_memcpy(void *dest, const void *src, unsigned short count);

/**
 * @brief  strcpy, end with '\0'
 * @param[in] dest
 * @param[in] src
 * @return dest
 */
char *my_strcpy(char *dest, const char *src);

/**
 * @brief  strcmp, end with '\0' or not equal
 * @param[in] s1
 * @param[in] s2
 * @return 0 is equal, -1 is not equal, -2 is error
 */
char my_strcmp(char *s1, char *s2);

/**
 * @brief  strncmp
 * @param[in] str1
 * @param[in] str2
 * @param[in] len
 * @return 0 is equal
 */
char my_strncmp(unsigned char *str1, unsigned char *str2, unsigned char len);

/**
 * @brief  convert hex to BCD
 * @param[in] Value_H
 * @param[in] Value_L
 * @return BCD type data
 */
unsigned char hex_to_bcd(unsigned char Value_H, unsigned char Value_L);

/**
 * @brief  convert int (32-bit) to byte array (8-bit * 4)
 *         this function is not safe
 * @param[in] number
 * @param[in] array
 * @return none
 */
void int_to_byte(unsigned long number, unsigned char *array);

/**
 * @brief  convert byte array (8-bit * 4) to int (32-bit)
 *         this function is not safe
 * @param[in] array
 * @return int (32-bit)
 */
unsigned long byte_to_int(const unsigned char *array);

/**
 * @brief  convert ASCII to hex
 * @param[in] ascii_data: the ASCII data
 * @return -1 means ascii_data is not "number", else means hex data
 */
char ascii_to_hex(unsigned char ascii_data);

/*----------------------------------------------------------
 *                     queue function
 *--------------------------------------------------------*/

/**
 * @brief  dequeue a byte
 * @param[out] value
 * @return operate result
 */
unsigned char queue_dequeue_byte(unsigned char *value);

/**
 * @brief  enqueue a byte
 * @param[out] value
 * @return operate result
 */
unsigned char queue_enqueue_byte(unsigned char value);

/*----------------------------------------------------------
 *                     framing function
 *--------------------------------------------------------*/

/**
 * @brief  fill frame payload, a byte
 * @param[in out] payload_len
 * @param[in] byte
 * @return none
 */
void uart_framing_fill_payload_byte(unsigned short *payload_len, unsigned char byte);

/**
 * @brief  fill frame payload, bytes
 * @param[in out] payload_len
 * @param[in] buff
 * @param[in] buff_len
 * @return none
 */
void uart_framing_fill_payload_buff(unsigned short *payload_len, unsigned char *buff, unsigned short buff_len);

#if (DEVICE_TYPE == SCENE_SWITCH_DEVICE)
/**
 * @brief  fill frame payload, group id (2 bytes)
 * @param[in out] payload_len
 * @return none
 */
void uart_framing_fill_payload_add_group_id(unsigned short *payload_len);
#endif

/**
 * @brief  fill frame protocol field
 * @param[in] cmd_id
 * @param[in] payload_len
 * @param[in] seq: if NULL, get a new frame seq
 * @return operate result
 */
unsigned char uart_framing_fill_protocol_field(unsigned char cmd_id, unsigned short payload_len, unsigned short *seq);

#ifdef __cplusplus
}
#endif

#endif
/* -END OF FILE-  */
