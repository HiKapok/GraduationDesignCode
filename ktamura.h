#ifndef KTAMURA_H
#define KTAMURA_H

#include "gdal_priv.h"
#include "cpl_conv.h"

#include <QString>
#include <QObject>

class KTamura : public QObject
{
public:
    KTamura(QString);
    ~KTamura();
    void build();
    double getCoarseness(){ return m_dCoarseness; }
    double getContrast(){ return m_dContrast; }
    double getDirectionality(){ return m_dDirectionality; }
    QString getSVMString(int=1);
private:
    QString m_sOutputRoot;
    QString m_sFileNoExtName;
    float * m_fOrgArray;
    float * m_fExtArray;
    double m_dCoarseness;
    double m_dContrast;
    double m_dDirectionality;
    long m_lXSize;
    long m_lYSize;
    const int m_iMaxExtend;
    bool reflectExtend(float *, float *);
    double calCoarseness();
    double calContrast();
    double calDirectionality();
    int getNextValleyPeak(long*,int,int,bool=false);
    double getAverageGray(float *,unsigned long int,unsigned long int,unsigned long int,long);
    QString getDirRoot(QString);

};

#endif // KTAMURA_H
