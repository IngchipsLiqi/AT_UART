
#ifndef BAEC_CRC_H__
#define BAEC_CRC_H__


#include <stdint.h>
#include <string.h>

/*
* The CRC parameters. Currently configured for CCITT.
* Simply modify these to switch to another CRC Standard.
*/
/*
#define POLYNOMIAL          0x8005
#define INITIAL_REMAINDER   0x0000
#define FINAL_XOR_VALUE     0x0000
*/
#define POLYNOMIAL          0x1021
#define INITIAL_REMAINDER   0xFFFF
#define FINAL_XOR_VALUE     0x0000

/*
#define POLYNOMIAL          0x1021
#define POLYNOMIAL          0xA001
#define INITIAL_REMAINDER   0xFFFF
#define FINAL_XOR_VALUE     0x0000
*/

/*
* The width of the CRC calculation and result.
* Modify the typedef for an 8 or 32-bit CRC standard.
*/
typedef unsigned short width_t;
#define WIDTH (8 * sizeof(width_t))
#define TOPBIT (1 << (WIDTH - 1))

/**
 * Initialize the CRC lookup table.
 * This table is used by crcCompute() to make CRC computation faster.
 */
void crcInit(void);

/**
 * Compute the CRC checksum of a binary message block.
 * @para message, ???????
 * @para nBytes, ?????
 * @note This function expects that crcInit() has been called
 *       first to initialize the CRC lookup table.
 */
width_t crcCompute(unsigned char * message, unsigned int nBytes);


#endif

