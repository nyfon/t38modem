/*
 * $Id: dle.cxx,v 1.1 2002-01-01 23:06:54 craigs Exp $
 *
 * T38FAX Pseudo Modem
 *
 * Original author: Vyacheslav Frolov
 *
 * $Log: dle.cxx,v $
 * Revision 1.1  2002-01-01 23:06:54  craigs
 * Initial version
 *
 * Revision 1.1  2002/01/01 23:06:54  craigs
 * Initial version
 *
 */

#include "dle.h"

///////////////////////////////////////////////////////////////

#define new PNEW

///////////////////////////////////////////////////////////////
static BYTE BitRevTable[256];

static BOOL initBitRevTable()
{
  for( unsigned i = 0 ; i < sizeof(BitRevTable) ; i++ ) {
    unsigned in = i, out = 0;
    for( int j = 0 ; j < 8 ; j++ ) {
      out <<= 1;
      out += in & 1;
      in >>= 1;
    }
    BitRevTable[i] = (BYTE)out;
  }
  return TRUE;
}

static const BOOL ___InitBitRevTable = initBitRevTable();
///////////////////////////////////////////////////////////////
enum {
  ETX = 0x03,
  DLE = 0x10,
};
///////////////////////////////////////////////////////////////
int DLEData::PutDleData(const void *pBuf, PINDEX count)
{
  if( eof ) return -1;
  
  PINDEX cRest = count;
  const BYTE *p = (const BYTE *)pBuf;
  
  while( cRest > 0 ) {
    const BYTE *pScan = p;
    PINDEX cScan = cRest;
    
    if( dle ) {
      dle = FALSE;
      if( *p == ETX ) {
        PutEof();
        cRest--;
        break;
      }
      pScan++;
      cScan--;
    }
    
    const BYTE *pDle = (const BYTE *)memchr(pScan, DLE, cScan);
    
    PINDEX cPut;
    PINDEX cDone;
  
    if( pDle ) {
      dle = TRUE;
      cPut = pDle - p;
      cDone = cPut + 1;	// skip DLE
    } else {
      cDone = cPut = cRest;
    }
  
    if( cPut ) {
      if( bitRev ) {
        BYTE tmp[1024];
        while( cPut ) {
          PINDEX cTmp = cPut > 1024 ? 1024 : cPut;
          for( PINDEX i = 0 ; i < cTmp ; i++ ) {
            tmp[i] = BitRevTable[p[i]];
          }
          PutData(tmp, cTmp);
          cPut -= cTmp;
        }
      } else {
        PutData(p, cPut);
      }
    }
    p += cDone;
    cRest -= cDone;
  }
  
  return count - cRest;
}

int DLEData::GetDleData(void *pBuf, PINDEX count)
{
  if( recvEtx ) return -1;

  BYTE *p = (BYTE *)pBuf;
  
  while( (count - (p - (BYTE *)pBuf)) >= 4 ) {
    PINDEX cGet = (count - 2) / 2;
    if( cGet > 1024 )
      cGet = 1024;
    
    BYTE tmp[1024];
    
    switch( cGet = GetData(tmp, cGet) ) {
      case -1:
        *p++ = DLE;
        *p++ = ETX;
        recvEtx = TRUE;
        return p - (BYTE *)pBuf;
      case 0:
        return p - (BYTE *)pBuf;
      default:
        for( PINDEX i = 0 ; i < cGet ; i++ ) {
          BYTE b = bitRev ? BitRevTable[tmp[i]] : tmp[i];
          if( b == DLE )
            *p++ = DLE;
          *p++ = b;
        }
    }
  }
  return p - (BYTE *)pBuf;
}
///////////////////////////////////////////////////////////////

