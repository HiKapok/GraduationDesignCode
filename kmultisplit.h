#ifndef KMULTISPLIT_H
#define KMULTISPLIT_H

#include "kregion.h"
#include <QString>
#include <vector>

class KMultiSplit
{
public:
    typedef enum {
        OutPic = 0,
        OutXML = 1,
    } K_OutTypes;
    ~KMultiSplit();
    KMultiSplit(QString,QString,QString,K_OutTypes=OutPic);
    void quickSplit(float);
private:
    QString m_sInput;
    QString m_sOutPut;
    QString m_sOutPutLabel;
    K_OutTypes m_tTypeOut;
    int m_nBand;
    int m_iXSize;
    int m_iYSize;
    float *m_imgBuff;
    float **m_orgImgBuff;
    long *labels;
    //float m_quickSplitThres;
    int getQuickColorMean(KRegion&,double *);
    float getQuickColorDiff(KRegion&,KRegion &);
};

#endif // KMULTISPLIT_H
