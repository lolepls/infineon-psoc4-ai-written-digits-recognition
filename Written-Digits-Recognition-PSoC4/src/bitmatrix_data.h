/*
 * bitmatrix_data.h
 *
 *  Created on: 25 lug 2023
 *      Author: Mombelli
 */

#ifndef SRC_BITMATRIX_DATA_H_
#define SRC_BITMATRIX_DATA_H_

// Define the structure (112x112 bits) with packed attribute
typedef struct __attribute__((packed)) {
    uint16_t data[112][7];
} BitMatrix112x112;

#endif /* SRC_BITMATRIX_DATA_H_ */
