/*
 * intensity_LUT.h
 *
 *  Created on: 26 lug 2023
 *      Author: Gioele Mombelli (IFI EMEA SMD TMS)
 */

#ifndef SRC_INTENSITY_LUT_H_
#define SRC_INTENSITY_LUT_H_


// Lookup table for intensity values
static const uint8_t intensity_table[17] = {
    0,   // 0 white pixels, intensity 0
    17,  // 1 white pixel, intensity (255 / 15) = 17
    34,  // 2 white pixels, intensity 2 * (255 / 15) = 34
    51,  // 3 white pixels, intensity 3 * (255 / 15) = 51
    68,  // 4 white pixels, intensity 4 * (255 / 15) = 68
    85,  // 5 white pixels, intensity 5 * (255 / 15) = 85
    102, // 6 white pixels, intensity 6 * (255 / 15) = 102
    119, // 7 white pixels, intensity 7 * (255 / 15) = 119
    136, // 8 white pixels, intensity 8 * (255 / 15) = 136
    170, // 9 white pixels, intensity 9 * (255 / 15) = 170
    204, // 10 white pixels, intensity 10 * (255 / 15) = 204
    238, // 11 white pixels, intensity 11 * (255 / 15) = 238
    255, // 12 white pixels, intensity 12 * (255 / 15) = 255
    255, // 13 white pixels, intensity 12 * (255 / 15) = 255
    255, // 14 white pixels, intensity 12 * (255 / 15) = 255
    255,  // 15 white pixels, intensity 12 * (255 / 15) = 255
	255  // 16 white pixels, intensity 12 * (255 / 15) = 255
};


#endif /* SRC_INTENSITY_LUT_H_ */
