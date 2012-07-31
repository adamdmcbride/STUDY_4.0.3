/**
 * @addtogroup app_XMLConfigParser
 * @{
 */

/***************************************************************************
 *                                                                         *
 *                   SiRF Technology, Inc. GPS Software                    *
 *                                                                         *
 *    Copyright (c) 2008-2009 by SiRF Technology, Inc. All rights reserved.*
 *                                                                         *
 *    This Software is protected by United States copyright laws and       *
 *    international treaties.  You may not reverse engineer, decompile     *
 *    or disassemble this Software.                                        *
 *                                                                         *
 *    WARNING:                                                             *
 *    This Software contains SiRF Technology Inc.s confidential and        *
 *    proprietary information. UNAUTHORIZED COPYING, USE, DISTRIBUTION,    *
 *    PUBLICATION, TRANSFER, SALE, RENTAL OR DISCLOSURE IS PROHIBITED      *
 *    AND MAY RESULT IN SERIOUS LEGAL CONSEQUENCES.  Do not copy this      *
 *    Software without SiRF Technology, Inc.s  express written             *
 *    permission.   Use of any portion of the contents of this Software    *
 *    is subject to and restricted by your signed written agreement with   *
 *    SiRF Technology, Inc.                                                *
 *                                                                         *
 **************************************************************************/

#ifndef XMLCONFIGPARSER_H
#define XMLCONFIGPARSER_H

#include  <stdio.h>
#include  <math.h>
#include  <stdlib.h>
#include  <string.h>

#ifdef __cplusplus 
extern "C" 
{ 
#endif

/*------------------------------------------------------------------*/
/* char* FindWordInConfigString ( unsigned int configStringLength ,
                              const char* configString, 
                              unsigned int  WordLength,
                              const char* Word )
        This function searches a Word in a string and returns the 
        first address of it.
  
        First address of Word in case of success.
        NULL in case of failure.
*/
/*------------------------------------------------------------------*/

const char* FindWordInConfigString ( unsigned int configStringLength ,
                                     const char* configString, 
                                     unsigned int  WordLength,
                                     const char* Word );

/*------------------------------------------------------------------*/
/*
   char* FindTagInConfigString ( unsigned int configStringLength,
                                     const char* configString, 
                                     unsigned int WordLength,
                                     const char* Word,
                                     unsigned int* TagLength )
        This function searches a tag like "<Word>data</Word>"
        in a string and returns the first address and length of it.
   
        First address of tag in case of success.
        NULL in case of failure.
*/
/*------------------------------------------------------------------*/
const char* FindTagInConfigString ( unsigned int configStringLength,
                                    const char* configString, 
                                    unsigned int WordLength,
                                    const char* Word,
                                    unsigned int* TagLength );

/*------------------------------------------------------------------*/
/*
    char* DataTagLengthAddressInConfigString( unsigned int WordLength,
                               char* TagAdr,
                               unsigned int TagLength,
                               unsigned int* DataLength )
        This function returns the data address and length of a 
        tag.
  
        Address of tag data in case of success.
        NULL in case of failure.
*/
/*------------------------------------------------------------------*/
const char* DataTagLengthAddressInConfigString( unsigned int WordLength,
                                                const char* TagAdr,
                                                unsigned int TagLength,
                                                unsigned int* DataLength );

/*------------------------------------------------------------------*/
/*
    char* FindTagContentInConfigString ( unsigned int configStringLength,
                                    const char* configString, 
                                    unsigned int WordLength,
                                    const char* Word,
                                    unsigned int* ContentLength )
        This function searches the first tag like "<Word>content</Word>"
        in a string and returns the address and length of its content.
    \param [in] vp_StringLength 
    \param [in] pp_String 
    \param [in] vp_WordLength 
    \param [in] pp_Word 
    \param [out] pp_ContentLength 
    \return 
        Address of tag content in case of success.
        NULL in case of failure.
*/
/*------------------------------------------------------------------*/
const char* FindTagContentInConfigString ( unsigned int configStringLength,
                                           const char* configString, 
                                           unsigned int WordLength,
                                           const char* Word,
                                           unsigned int* ContentLength );

#ifdef __cplusplus 
} 
#endif

#endif /* XMLCONFIGPARSER_H */

/**
 * @}
 */
