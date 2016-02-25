#ifndef __COMMON_H__
#define __COMMON_H__

#include "gdal_priv.h"
#include "cpl_conv.h"

#include <iostream>
#include <Qdebug>

#define K_OPEN_ASSERT(set,file) if( set == NULL ){\
                                    std::cout<<"file "<<file<<" open failed\a"<<std::endl;\
                                    GDALDestroyDriverManager();\
                                    exit( 1 );}

extern void calFastHistogram( GDALRasterBand *poBand, GUIntBig *panHistogram );
extern void calHistogram( GDALRasterBand *poBand, GUIntBig *panHistogram );
extern bool K_CheckDataSetEqu(GDALDataset *set1, GDALDataset *set2);

#endif /* __COMMON_H__ */

