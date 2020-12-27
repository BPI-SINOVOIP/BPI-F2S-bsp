/*
 * GPIO Driver for Sunplus I143 controller
 * Copyright (C) 2020 Sunplus Tech./Tibbo Tech.
 * Author: Dvorkin Dmitry <dvorkin@tibbo.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "sppctl_gpio.h"


const char * const sppctlgpio_list_s[] = {
	D_PIS(  0), D_PIS(  1), D_PIS(  2), D_PIS(  3), D_PIS(  4), D_PIS(  5), D_PIS(  6), D_PIS(  7),
	D_PIS(  8), D_PIS(  9), D_PIS( 10), D_PIS( 11), D_PIS( 12), D_PIS( 13), D_PIS( 14), D_PIS( 15),
	D_PIS( 16), D_PIS( 17), D_PIS( 18), D_PIS( 19), D_PIS( 20), D_PIS( 21), D_PIS( 22), D_PIS( 23),
	D_PIS( 24), D_PIS( 25), D_PIS( 26), D_PIS( 27), D_PIS( 28), D_PIS( 29), D_PIS( 30), D_PIS( 31),
	D_PIS( 32), D_PIS( 33), D_PIS( 34), D_PIS( 35), D_PIS( 36), D_PIS( 37), D_PIS( 38), D_PIS( 39),
	D_PIS( 40), D_PIS( 41), D_PIS( 42), D_PIS( 43), D_PIS( 44), D_PIS( 45), D_PIS( 46), D_PIS( 47),
	D_PIS( 48), D_PIS( 49), D_PIS( 50), D_PIS( 51), D_PIS( 52), D_PIS( 53), D_PIS( 54), D_PIS( 55),
	D_PIS( 56), D_PIS( 57), D_PIS( 58), D_PIS( 59), D_PIS( 60), D_PIS( 61), D_PIS( 62), D_PIS( 63),
	D_PIS( 64), D_PIS( 65), D_PIS( 66), D_PIS( 67), D_PIS( 68), D_PIS( 69), D_PIS( 70), D_PIS( 71),
	D_PIS( 72), D_PIS( 73), D_PIS( 74), D_PIS( 75), D_PIS( 76), D_PIS( 77), D_PIS( 78), D_PIS( 79),
	D_PIS( 80), D_PIS( 81), D_PIS( 82), D_PIS( 83), D_PIS( 84), D_PIS( 85), D_PIS( 86), D_PIS( 87),
	D_PIS( 88), D_PIS( 89), D_PIS( 90), D_PIS( 91), D_PIS( 92), D_PIS( 93), D_PIS( 94), D_PIS( 95),
	D_PIS( 96), D_PIS( 97), D_PIS( 98), D_PIS( 99), D_PIS(100), D_PIS(101), D_PIS(102), D_PIS(103),
	D_PIS(104), D_PIS(105), D_PIS(106), D_PIS(107)
};

const size_t GPIS_listSZ = sizeof(sppctlgpio_list_s)/sizeof(*(sppctlgpio_list_s));
