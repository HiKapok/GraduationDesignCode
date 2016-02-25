#ifndef __COMMON_H__
#define __COMMON_H__

#include "gdal_priv.h"
#include "cpl_conv.h"

#include <cassert>
#include <iostream>
#include <Qdebug>

#define K_OPEN_ASSERT(set,file) if( set == NULL ){\
                                    std::cout<<"file "<<file<<" open failed\a"<<std::endl;\
                                    GDALDestroyDriverManager();\
                                    exit( 1 );}
namespace Kapok{
    /* Pixel border types */
    typedef enum {
        /* default border type */                   Border_Default = 1,
        /* gfedcb|abcdefgh|gfedcba */               Border_Reflect = 1,
        /* iiiiii|abcdefgh|iiiiiii */               Border_Constant = 2,
        /* aaaaaa|abcdefgh|hhhhhhh */               Border_Replicate = 3,
        /* the end */                               Border_End = 4
    } K_BorderTypes;
}

extern "C" void calFastHistogram( GDALRasterBand *poBand, GUIntBig *panHistogram );
extern "C" void calHistogram( GDALRasterBand *poBand, GUIntBig *panHistogram );
extern "C" bool K_CheckDataSetEqu(GDALDataset *set1, GDALDataset *set2);
extern "C" GUIntBig get2Power(GByte pow);

#endif /* __COMMON_H__ */

