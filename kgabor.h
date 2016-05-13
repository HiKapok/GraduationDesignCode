#ifndef KGABOR_H
#define KGABOR_H

#include "gdal_priv.h"
#include "cpl_conv.h"

#include "kprogressbar.h"

#include <QString>

#include <tuple>
#include <vector>

class KGabor
{
public:
    KGabor(QString,int=15,int=8);
    ~KGabor();
    void build();
    QString getSVMString(int=1);
private:
    QString m_sOutputRoot;
    QString m_sFileNoExtName;
    float * m_fOrgArray;
    float * m_fExtArray;
    float * m_fGaborFilter;
    float * m_fImagGaborFilter;
    int m_iXSize;
    int m_iYSize;
    int m_extImageWidth;
    int m_iKernelSize;
    const int m_iKernelRadius;
    int m_iMaxDegree;
    std::vector<float> m_vecParam;
    void getGaborFilter(int,int,GDALDataset * =NULL,bool=false);
    void printFilter(float*,QString);
    void printFilter(GDALDataset*,float*,int,int);
    void printImage(float*,QString);
};

#endif // KGABOR_H

