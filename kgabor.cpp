#include "kgabor.h"

#include "common.h"
#include "kutility.h"
#include "kimagecvt.h" //for image convert

#include <QRegularExpression>
#include <QDebug>
#include <QDir>

#include <cmath>

KGabor::KGabor(QString input,int kernelSize)
     :m_iKernelSize((kernelSize+1)/2)
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
    m_fExtArray = (float *) CPLMalloc(sizeof(float)*(m_iXSize+2*m_iKernelSize)*(m_iYSize+2*m_iKernelSize));

    // gray only
    GDALRasterBand * poBand = poDataset->GetRasterBand(1);
    poBand->RasterIO( GF_Read, 0, 0, m_iXSize, m_iYSize, m_fOrgArray, m_iXSize, m_iYSize, GDT_Float32, 0, 0 );

    KUtility::reflectExtend<float>(m_fOrgArray,m_fExtArray,m_iXSize,m_iYSize,m_iKernelSize);

    m_extImageWidth=m_iXSize + 2*m_iKernelSize;

    GDALClose(piDataset);
    GDALClose(poDataset);
}

KGabor::~KGabor()
{
    CPLFree(m_fOrgArray);
    CPLFree(m_fExtArray);
}

void KGabor::build()
{
    int totalPixels = m_iXSize*m_iYSize;

    KProgressBar progressBar("calcGabor",100,80);
    K_PROGRESS_START(progressBar);



    K_PROGRESS_END(progressBar);

}

QString KGabor::getSVMString(int start)
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

