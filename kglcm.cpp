#include "kglcm.h"

#include "common.h"
#include "kutility.h"
#include "kimagecvt.h" //for image convert

#include <QRegularExpression>
#include <QDebug>
#include <QDir>

#include <cmath>

KGLCM::KGLCM(QString input,int distance,int grayLevel)
     :m_iDistance(distance),
      m_iGrayLevel(grayLevel)
{
    QRegularExpression re("[\\\\/]");

    GDALDataset * piDataset = (GDALDataset *) GDALOpen(input.toUtf8().constData(), GA_ReadOnly );

    K_OPEN_ASSERT(piDataset,input.toStdString());

    m_sOutputRoot = KUtility::getDirRoot(input);

    QString tempName=input.right(input.length()-input.lastIndexOf(re)-1);
    m_sFileNoExtName=tempName.left(tempName.lastIndexOf("."));

    GDALDataset *poDataset = NULL;

    if((poDataset = KImageCvt::img2gray(piDataset,poDataset,m_sOutputRoot+"temp"+QDir::separator()+m_sFileNoExtName+"-gray")) == NULL) { std::cout<<"image convert failed\a"<<std::endl; exit(1); }

    m_iXSize = piDataset->GetRasterXSize();
    m_iYSize = piDataset->GetRasterYSize();
    m_fOrgArray = (float *) CPLMalloc(sizeof(float)*m_iXSize*m_iYSize);
    m_iExtArray = (int *) CPLMalloc(sizeof(int)*(m_iXSize+2*m_iDistance)*(m_iYSize+2*m_iDistance));

    // gray only
    GDALRasterBand * poBand = poDataset->GetRasterBand(1);
    poBand->RasterIO( GF_Read, 0, 0, m_iXSize, m_iYSize, m_fOrgArray, m_iXSize, m_iYSize, GDT_Float32, 0, 0 );

    KUtility::reflectExtend<int>(m_fOrgArray,m_iExtArray,m_iXSize,m_iYSize,m_iDistance);

    m_extImageWidth=m_iXSize + 2*m_iDistance;
    for(int iY = 0;iY < m_iYSize + 2*m_iDistance;++iY){
        for(int iX = 0;iX < m_extImageWidth;++iX){
            m_iExtArray[iY*m_extImageWidth+iX] = m_iExtArray[iY*m_extImageWidth+iX] / m_iGrayLevel * m_iGrayLevel + m_iGrayLevel/2;
        }
    }

    m_iArrGLCM = (int *) CPLMalloc(sizeof(int)*m_iGrayLevel*m_iGrayLevel);
    memset(m_iArrGLCM,0,sizeof(int)*m_iGrayLevel*m_iGrayLevel);

    GDALClose(piDataset);
    GDALClose(poDataset);

    m_vecParam.clear();
}

KGLCM::~KGLCM()
{
    CPLFree(m_fOrgArray);
    CPLFree(m_iExtArray);
    CPLFree(m_iArrGLCM);
}

void KGLCM::build()
{
    int totalPixels = m_iXSize*m_iYSize;

    KProgressBar progressBar("calcGLCM",(G_End-1)*(totalPixels+3*m_iGrayLevel+5*m_iGrayLevel*m_iGrayLevel),80);
    K_PROGRESS_START(progressBar);

    m_fArrGLCM = (float *) CPLMalloc(sizeof(float)*m_iGrayLevel*m_iGrayLevel);

    for(int index=G_Start;index<G_End;++index){
        calcGLCM(K_GLCM_Degree(index),&progressBar);

        for(int iY=0;iY<m_iGrayLevel;++iY){
            for(int iX=0;iX<m_iGrayLevel;++iX){
                m_fArrGLCM[iY*m_iGrayLevel+iX]=m_iArrGLCM[iY*m_iGrayLevel+iX]*1./totalPixels;
                progressBar.autoUpdate();
            }
        }
//        for(int iY=0;iY<m_iGrayLevel*m_iGrayLevel;++iY)
//        qDebug()<<m_iArrGLCM[iY];
//        for(int iY=0;iY<m_iGrayLevel*m_iGrayLevel;++iY)
//        qDebug()<<m_fArrGLCM[iY];
        m_vecParam.push_back(getContrast(&progressBar));
        m_vecParam.push_back(getEnergy(&progressBar));
        m_vecParam.push_back(getEntropy(&progressBar));
        m_vecParam.push_back(getCorrelation(&progressBar));
        m_vecParam.push_back(getHomogeneity(&progressBar));

        memset(m_iArrGLCM,0,sizeof(int)*m_iGrayLevel*m_iGrayLevel);
    }

    K_PROGRESS_END(progressBar);
    CPLFree(m_fArrGLCM);
}

QString KGLCM::getSVMString(int start)
{
    QString temp("");
    for(int index= start;index<start+25;++index){
        temp+=QString("%1:%%2 ").arg(index).arg(index-start+1);
    }
    for(auto _vec : m_vecParam){
        temp = QString(temp).arg(_vec);
    }
    return temp;
}

void KGLCM::calcGLCM(KGLCM::K_GLCM_Degree degree,KProgressBar *progressBar)
{
    for(int iY = m_iDistance;iY < m_iYSize+m_iDistance;++iY){
        for(int iX = m_iDistance;iX < m_iXSize+m_iDistance;++iX){
            auto gray = getGrayByDir(std::make_tuple(iY,iX),degree);
            int thisGray = m_iExtArray[iY*m_extImageWidth+iX];
            thisGray /= m_iGrayLevel;
            m_iArrGLCM[thisGray*m_iDistance+std::get<0>(gray)/m_iGrayLevel]++;
            m_iArrGLCM[thisGray*m_iDistance+std::get<1>(gray)/m_iGrayLevel]++;
            if(progressBar != NULL) progressBar->autoUpdate();
        }
    }
}

std::tuple<int,int> KGLCM::getGrayByDir(std::tuple<int,int> point, KGLCM::K_GLCM_Degree degree)
{
    int tempGrayPositive = 0;
    int tempGrayMinus = 0;
    int positionX = 0;
    int positionY = 0;
    if(G_Horizontal == degree){
        positionY = std::get<0>(point);
        positionX = std::get<1>(point)-m_iDistance;
        tempGrayMinus = m_iExtArray[positionY*m_extImageWidth+positionX];
        positionX += 2*m_iDistance;
        tempGrayPositive = m_iExtArray[positionY*m_extImageWidth+positionX];
    }else{
        if(G_Vertical == degree){
            positionY = std::get<0>(point)-m_iDistance;
            positionX = std::get<1>(point);
            tempGrayMinus = m_iExtArray[positionY*m_extImageWidth+positionX];
            positionY += 2*m_iDistance;
            tempGrayPositive = m_iExtArray[positionY*m_extImageWidth+positionX];
        }else{
            if(G_LeftRight == degree){
                positionY = std::get<0>(point)-m_iDistance;
                positionX = std::get<1>(point)-m_iDistance;
                tempGrayMinus = m_iExtArray[positionY*m_extImageWidth+positionX];
                positionY += 2*m_iDistance;
                positionX += 2*m_iDistance;
                tempGrayPositive = m_iExtArray[positionY*m_extImageWidth+positionX];
            }else{
                positionY = std::get<0>(point)+m_iDistance;
                positionX = std::get<1>(point)-m_iDistance;
                tempGrayMinus = m_iExtArray[positionY*m_extImageWidth+positionX];
                positionY -= 2*m_iDistance;
                positionX += 2*m_iDistance;
                tempGrayPositive = m_iExtArray[positionY*m_extImageWidth+positionX];
            }
        }
    }
    return std::make_tuple(tempGrayMinus,tempGrayPositive);
}

float KGLCM::getContrast(KProgressBar * progress)
{
    float fContrast=0.;
    for(int iY=0;iY<m_iGrayLevel;++iY){
        for(int iX=0;iX<m_iGrayLevel;++iX){
            float temp = std::abs(iX-iY);
            fContrast += temp*temp*m_fArrGLCM[iY*m_iGrayLevel+iX];
            progress->autoUpdate();
        }
    }
    return fContrast;
}

float KGLCM::getEnergy(KProgressBar * progress)
{
    float fEnergy=0.;
    for(int iY=0;iY<m_iGrayLevel;++iY){
        for(int iX=0;iX<m_iGrayLevel;++iX){
            float temp = m_fArrGLCM[iY*m_iGrayLevel+iX];
            fEnergy += temp*temp;
            progress->autoUpdate();
        }
    }
    return fEnergy;
}

float KGLCM::getEntropy(KProgressBar * progress)
{
    float fEntropy=0.;
    for(int iY=0;iY<m_iGrayLevel;++iY){
        for(int iX=0;iX<m_iGrayLevel;++iX){
            float temp = m_fArrGLCM[iY*m_iGrayLevel+iX];
            // minValue limiter
            //if(temp<1./(m_iXSize*m_iYSize)) temp=0.5/(m_iXSize*m_iYSize);
            // don't contain too minValue
            if(temp<1./(m_iXSize*m_iYSize)) continue;
            fEntropy -= temp*1.*log2(temp);
            progress->autoUpdate();
        }
    }
    return fEntropy;

}

float KGLCM::getCorrelation(KProgressBar * progress)
{
    float fCorrelation=0.;
    float fMr=0.;
    float fMc=0.;

    float fDeltar=0.;
    float fDeltac=0.;

    for(int iY=0;iY<m_iGrayLevel;++iY){
        for(int iX=0;iX<m_iGrayLevel;++iX){
            fMr += iY * m_fArrGLCM[iY*m_iGrayLevel+iX];
            fMc += iX * m_fArrGLCM[iY*m_iGrayLevel+iX];
        }
        progress->autoUpdate();
    }
    for(int iY=0;iY<m_iGrayLevel;++iY){
        for(int iX=0;iX<m_iGrayLevel;++iX){
            fDeltar += (iY-fMr) * (iY-fMr) * m_fArrGLCM[iY*m_iGrayLevel+iX];
            fDeltac += (iX-fMc) * (iX-fMc) * m_fArrGLCM[iY*m_iGrayLevel+iX];
        }
        progress->autoUpdate();
    }
    fDeltar = pow(fDeltar,0.5);
    fDeltac = pow(fDeltac,0.5);
    if(fDeltar<0.0001) fDeltar=0.0001;
    if(fDeltac<0.0001) fDeltac=0.0001;

    for(int iY=0;iY<m_iGrayLevel;++iY){
        for(int iX=0;iX<m_iGrayLevel;++iX){
            fCorrelation += (iY-fMr) * (iX-fMc) * m_fArrGLCM[iY*m_iGrayLevel+iX];
        }
        progress->autoUpdate();
    }
    fCorrelation=fCorrelation/(fDeltar*fDeltac);
    return fCorrelation;
}

float KGLCM::getHomogeneity(KProgressBar * progress)
{
    float fHomogeneity=0.;
    for(int iY=0;iY<m_iGrayLevel;++iY){
        for(int iX=0;iX<m_iGrayLevel;++iX){
            float temp = std::abs(iX-iY);
            fHomogeneity += m_fArrGLCM[iY*m_iGrayLevel+iX]/(1.+temp);
            progress->autoUpdate();
        }
    }
    return fHomogeneity;
}
