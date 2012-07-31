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

#include "XmlConfigParser.h"

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
                               const char* Word )
{
   unsigned int pos;
  
   if ( ( WordLength > configStringLength ) || 
        ( configString == NULL ) || 
        ( Word == NULL ) )
   {
      return NULL;
   }
  
   for ( pos = 0; pos <= configStringLength-WordLength; pos++ )
   {
      if ( 0 == strncmp( configString+pos, Word, WordLength ) )
      {
         return (const char*)(configString+pos);
      }
   }
  
   return NULL;
} 


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
                                    unsigned int* TagLength )
{
   char StartTag[40];
   char StopTag[40];
   const char*  StartAdr;
   const char*  StopAdr;

   if ( ( configString == NULL ) ||
        ( Word == NULL ) ||
        ( TagLength == NULL ) )
   {
      return NULL;
   }

   StartTag[0] = '<';
   strncpy( &StartTag[1], Word, WordLength );
   StartTag[WordLength+1] = '>';
   StartTag[WordLength+2] = 0;

   StopTag[0] = '<';
   StopTag[1] = '/';
   strncpy( &StopTag[2], Word, WordLength );
   StopTag[WordLength+2] = '>';
   StopTag[WordLength+3] = 0;

   StartAdr = FindWordInConfigString( configStringLength, configString, WordLength+2, StartTag );
   if ( NULL == StartAdr)
   {
      return NULL;
   }

   StopAdr = FindWordInConfigString( configStringLength-(StartAdr-configString), StartAdr, WordLength+3, StopTag );
   if ( NULL == StopAdr)
   {
      return NULL;
   }

   *TagLength = StopAdr-StartAdr + WordLength+3;

   return StartAdr;
} 



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
                                                unsigned int* DataLength )
{
   const char* DataAdr;

   if ( TagLength < 2*WordLength+5 )
   {
      return NULL;
   }

   DataAdr = TagAdr+WordLength+2;
   *DataLength = TagLength-2*WordLength-5;

   return DataAdr;
} 


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
                                           unsigned int* ContentLength )
{
   const char* StartAdr;
   unsigned int   TagLength;

   StartAdr = FindTagInConfigString( configStringLength, configString, WordLength, Word, &TagLength);

   if (StartAdr != NULL )
   {
      StartAdr = DataTagLengthAddressInConfigString( WordLength, StartAdr, TagLength, ContentLength);
   }

   return StartAdr;
} 

/* Example of ConfigString
  <NumberOfStarts>1</NumberOfStarts>
   <StartType>Cold</StartType> 
   <FixType>Periodic</FixType> 
   <NumberOfFixes>1</NumberOfFixes>
   <FixPeriod>1sec</FixPeriod>
   <ServerAddr>192.192.0.1</ServerAddr>
   <Port>8000</Port> 
   <AgpsMode>Autonomous</AgpsMode> 
   <AssistProtocol>SUPL-RRLP</AssistProtocol>
   <NmeaMask>GGA,GSA,GSV</NmeaMask>
   <LoggingInformation>logFile.txt</LoggingInformation>
*/
/*
<GpsConfig><NumberOfStarts>5</NumberOfStarts><StartType>Hot</StartType><FixType>Single</FixType><NumberOfFixes>1</NumberOfFixes><FixPeriodMs>0</FixPeriodMs><AgpsMode>Autonomous</AgpsMode><AssistProtocol>SUPL-RRLP</AssistProtocol><NmeaMask>GGA</NmeaMask><ServerAddr>66.232.130.251</ServerAddr><Port>59910</Port></GpsConfig>
*/

#ifdef __cplusplus 
}
#endif

/**
 * @}
 */
