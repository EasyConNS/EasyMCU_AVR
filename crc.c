/**********************************************************************
 *
 * Filename:    crc.c
 *
 * Description: Source de calculo CRC
 *
 * Notes:
 *
 *
 *
 * Copyright (c) 2014 Francisco Javier Lana Romero
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
 
#include "crc.h"



/*
 * Derive parameters from the standard-specific parameters in crc.h.
 */
#ifndef WIDTH
    #define WIDTH    (8 * sizeof(crc))
#endif
#define TOPBIT   (((crc)1) << (WIDTH - 1))

#if (REVERSED_DATA == TRUE)
    #define FP_reflect_DATA(_DATO)                      ((uint8_t)(FP_reflect((_DATO), 8)&0xFF))
    #define FP_reflect_CRCTableValue(_CRCTableValue)	((crc) FP_reflect((_CRCTableValue), WIDTH))
#else
    #define FP_reflect_DATA(_DATO)                      (_DATO)
    #define FP_reflect_CRCTableValue(_CRCTableValue)	(_CRCTableValue)

#endif

#if (CALCULATE_LOOKUPTABLE == TRUE)
static crc  A_crcLookupTable[256] = {0};
#endif
/*********************************************************************
 *
 * Function:    FP_reflect()
 * 
 * Description: A value/register is reflected if it's bits are swapped around its centre.
 * For example: 0101 is the 4-bit reflection of 1010
 *
 * Descripción: Un valor es reflejado cuando sus bits son intercambiados utilizando como punto de referencia el centro.
 * Por ejemplo: 0101 es el reflejo de 1010
 *
 *********************************************************************/
 #if (REVERSED_DATA == TRUE)
static crc FP_reflect(crc VF_dato, uint8_t VF_nBits)
{
    crc VP_reflection = 0;
    uint8_t VP_Pos_bit = 0;

    for (VP_Pos_bit = 0; VP_Pos_bit < VF_nBits; VP_Pos_bit++)
    {
        if ((VF_dato & 1) == 1)
        {
            VP_reflection |= (((crc)1) << ((VF_nBits - 1) - VP_Pos_bit));
        }

        VF_dato = (VF_dato >> 1);
    }
    return (VP_reflection);
}

#endif

/*********************************************************************
 *
 * Function:    F_CRC_ObtenValorDeTabla()
 * 
 * Description: Obtains the Table value
 * Descripción: Obtiene el valor de la tabla
 *
 *
 *********************************************************************/
static crc F_CRC_ObtenValorDeTabla(uint8_t VP_Pos_Tabla)
{
    crc VP_CRCTableValue = 0;
    uint8_t VP_Pos_bit = 0;

    VP_CRCTableValue = ((crc) FP_reflect_DATA(VP_Pos_Tabla)) << (WIDTH - 8);

    for (VP_Pos_bit = 0; VP_Pos_bit < 8; VP_Pos_bit++)
    {
        if (VP_CRCTableValue & TOPBIT)
        {
            VP_CRCTableValue = (VP_CRCTableValue << 1) ^ POLYNOMIAL;
        }
        else
        {
            VP_CRCTableValue = (VP_CRCTableValue << 1);
        }
    }
    return (FP_reflect_CRCTableValue(VP_CRCTableValue));
}

#if (CALCULATE_LOOKUPTABLE == TRUE)

/*********************************************************************
 *
 * Function:    F_CRC_InicializaTabla()
 * 
 * Description: Create the lookup table for the CRC
 * Descripción: Crea la tabla de lookup para el computo del CRC
 *
 *
 *********************************************************************/
void F_CRC_InicializaTabla(void)
{
    uint16_t VP_Pos_Array = 0;

    for (VP_Pos_Array = 0; VP_Pos_Array < 256; VP_Pos_Array++)
    {
        A_crcLookupTable[VP_Pos_Array] = F_CRC_ObtenValorDeTabla((uint8_t)(VP_Pos_Array &0xFF));

    }

}



/*********************************************************************
 *
 * Function:    F_CRC_CalculaCheckSumDeTabla()
 * 
 * Description: Calculate the CRC value from a Lookup Table.
 *
 * Notes:		F_CRC_InicializaTabla() must be called first.
 *                      Since AF_Datos is a char array, it is possible to compute any kind of file or array.
 * Notes:		F_CRC_InicializaTabla() debe ser llamado primero
 *                      Como AF_Datos es un array de char, tiene capacidad para calcular la checksum de cualquier fichero o array.
 *
 * Returns:		The CRC of the AF_Datos.
 * Retorna:             El computo del CRC
 *
 *********************************************************************/
crc F_CRC_CalculaCheckSum(uint8_t const AF_Datos[], uint16_t VF_nBytes)
{
    crc	VP_CRCTableValue = INITIAL_VALUE;
    uint16_t VP_bytes = 0;

    for (VP_bytes = 0; VP_bytes < VF_nBytes; VP_bytes++)
    {
        #if (REVERSED_DATA == TRUE)
            VP_CRCTableValue = (VP_CRCTableValue >> 8) ^ A_crcLookupTable[((uint8_t)(VP_CRCTableValue & 0xFF)) ^ AF_Datos[VP_bytes]];
        #else
            VP_CRCTableValue = (VP_CRCTableValue << 8) ^ A_crcLookupTable[((uint8_t)((VP_CRCTableValue >> (WIDTH - 8)) & 0xFF)) ^ AF_Datos[VP_bytes]];
        #endif
    }
 
	if ((8 * sizeof(crc)) > WIDTH)
	{
		VP_CRCTableValue = VP_CRCTableValue & ((((crc)(1)) << WIDTH) - 1);
	}

    #if (REVERSED_OUT == FALSE)
        return (VP_CRCTableValue ^ FINAL_XOR_VALUE);
    #else
        return (~VP_CRCTableValue ^ FINAL_XOR_VALUE);
    #endif

}
#else


/*********************************************************************
 *
 * Function:    F_CRC_CalculaCheckSumSinTabla()
 *
 * Description: Calculate the CRC value withouth a lookup table
 * Description: Calculate el CRC sin tabla de lookup
 *
 * Notes:
 *
 * Returns:		The CRC of the AF_Datos.
 * Retorna:             El computo de CRC de AF_DAtos
 *
 *********************************************************************/
crc F_CRC_CalculaCheckSum(uint8_t const AF_Datos[], uint16_t VF_nBytes)
{
    crc	VP_CRCTableValue = INITIAL_VALUE;
    int16_t VP_bytes = 0;

    for (VP_bytes = 0; VP_bytes < VF_nBytes; VP_bytes++)
    {
        #if (REVERSED_DATA == TRUE)
            VP_CRCTableValue = (VP_CRCTableValue >> 8) ^ F_CRC_ObtenValorDeTabla(((uint8_t)(VP_CRCTableValue & 0xFF)) ^ AF_Datos[VP_bytes]);
        #else
            VP_CRCTableValue = (VP_CRCTableValue << 8) ^ F_CRC_ObtenValorDeTabla(((uint8_t)((VP_CRCTableValue >> (WIDTH - 8)) & 0xFF)) ^ AF_Datos[VP_bytes]);
        #endif
    }

#if 0
	if ((8 * sizeof(crc)) > WIDTH)
	{
		VP_CRCTableValue = VP_CRCTableValue & ((((crc)(1)) << WIDTH) - 1);
	}
#endif

    #if (REVERSED_OUT == FALSE)
        return (VP_CRCTableValue ^ FINAL_XOR_VALUE);
    #else
        return (~VP_CRCTableValue ^ FINAL_XOR_VALUE);
    #endif

}

#endif

