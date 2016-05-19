#ifndef KGLCM_H
#define KGLCM_H

#include "gdal_priv.h"
#include "cpl_conv.h"

#include "kprogressbar.h"

#include <QString>

#include <tuple>
#include <vector>

class KGLCM
{
    typedef enum {
        G_Start = 0,
        G_Horizontal = 1,
        G_Vertical = 2,
        G_LeftRight = 3,
        G_RightLeft = 4,
        G_End = 5,
    } K_GLCM_Degree;
public:
    KGLCM(QString,int=8,int=16,bool=false);
    ~KGLCM();
    void build();
    QString getSVMString(int=1);
private:
    QString m_sOutputRoot;
    QString m_sFileNoExtName;
    float * m_fOrgArray;
    int * m_iExtArray;
    int * m_iArrGLCM;
    float * m_fArrGLCM;
    int m_iXSize;
    int m_iYSize;
    int m_extImageWidth;
    const int m_iDistance;
    const int m_iGrayLevel;
    bool m_beRecorder;
    std::vector<float> m_vecParam;
    void calcGLCM(K_GLCM_Degree, KProgressBar* =NULL);
    std::tuple<int,int> getGrayByDir(std::tuple<int,int>, K_GLCM_Degree);
    float getContrast(KProgressBar* =NULL);
    float getEnergy(KProgressBar* =NULL);
    float getEntropy(KProgressBar* =NULL);
    float getCorrelation(KProgressBar* =NULL);
    float getHomogeneity(KProgressBar* =NULL);
};

#endif // KGLCM_H
