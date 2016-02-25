#ifndef KIMAGECVT_H
#define KIMAGECVT_H

#include "gdal_priv.h"
#include <QString>

class KImageCvt
{
public:
    KImageCvt();
public:
    static GDALDataset * img2gray(GDALDataset *,GDALDataset *,QString="");
    static GDALDataset * normalize(GDALDataset *,GDALDataset *,float,float,QString="");
    static GDALDataset * addWeighted(GDALDataset *,GDALDataset *,float,float,QString="");
    static GDALDataset * colorReduce(GDALDataset *,GDALDataset *,int,QString="");
};

#endif // KIMAGECVT_H
