#ifndef KFEATURELBP_H
#define KFEATURELBP_H

#include "gdal_priv.h"

#include "common.h"
#include "kprogressbar.h"

#include <QString>

class KFeatureLBP
{
public:
    ~KFeatureLBP();
    KFeatureLBP(GDALDataset *, GDALDataset *, int=8, int=1);
    inline GDALDataset * getDataSet() { return m_poDataset; }
    bool run(Kapok::K_BorderTypes=Kapok::Border_Default,bool=false);
    GDALDataset * build(QString="");
    QString getRealExtName(){ return m_realExtName; }
private:
    GDALDataset *m_piDataset;
    GDALDataset *m_piOrgDataset;
    GDALDataset *m_poDataset;
    int m_kernelRadius;
    int m_sampleNum;
    QString m_fileName;
    QString m_realExtName;
    unsigned char *refTable;
    bool calLBP(float * ,GByte *, int,int,bool,KProgressBar* =NULL);
    bool constExtend(float * ,float *,int,int,float);
    bool reflectExtend(float * ,float *,int,int);
    bool replicateExtend(float * ,float *,int,int);
    bool externDataSet(Kapok::K_BorderTypes,float=0);
};

#endif // KFEATURELBP_H
