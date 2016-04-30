#ifndef KFEATURELBP_H
#define KFEATURELBP_H

#include "gdal_priv.h"

#include "common.h"
#include "kprogressbar.h"

#include <QString>
#include <QObject>

class KFeatureLBP : public QObject
{
public:
    ~KFeatureLBP();
    KFeatureLBP(GDALDataset *, GDALDataset *, int=8, int=1, bool=true);
    inline GDALDataset * getDataSet() { return m_poDataset; }
    bool run(Kapok::K_BorderTypes=Kapok::Border_Default,bool=false);
    GDALDataset * build(QString="");
    QString getRealExtName(){ return m_realExtName; }
    QString getSVMString(int=1);
private:
    GDALDataset *m_piDataset;
    //GDALDataset *m_piOrgDataset;
    GDALDataset *m_poDataset;
    float **m_extDataBuff;
    int m_kernelRadius;
    int m_sampleNum;
    int m_bandNum;
    bool m_beImproved;
    QString m_fileName;
    QString m_realExtName;
    unsigned char *refTable;
    long *m_pHistogram;
   // QString tempExtFile;
    bool calLBP(float * ,GByte *, int,int,bool,KProgressBar* =NULL);
    bool constExtend(float * ,float *,int,int,float);
    bool reflectExtend(float * ,float *,int,int);
    bool replicateExtend(float * ,float *,int,int);
    bool externDataSet(Kapok::K_BorderTypes,float=0);

};

#endif // KFEATURELBP_H
