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
    KGabor(QString,int=32);
    ~KGabor();
    void build();
    QString getSVMString(int=1);
private:
    QString m_sOutputRoot;
    QString m_sFileNoExtName;
    float * m_fOrgArray;
    float * m_fExtArray;
    int m_iXSize;
    int m_iYSize;
    int m_extImageWidth;
    const int m_iKernelSize;
    std::vector<float> m_vecParam;
};

#endif // KGABOR_H

