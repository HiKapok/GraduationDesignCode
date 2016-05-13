#include "kgabor.h"

#include "common.h"
#include "kutility.h"
#include "kimagecvt.h" //for image convert

#include <QRegularExpression>
#include <QDebug>
#include <QDir>

#include <cmath>
#include <limits>

KGabor::KGabor(QString input,int kernelRadius,int maxDegree)
      :m_iKernelRadius(kernelRadius),
       m_iMaxDegree(maxDegree)
{
    QRegularExpression re("[\\\\/]");

    m_iKernelSize = 2*kernelRadius+1;
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
    m_fExtArray = (float *) CPLMalloc(sizeof(float)*(m_iXSize+2*m_iKernelRadius)*(m_iYSize+2*m_iKernelRadius));
    m_fGaborFilter = (float *) CPLMalloc(sizeof(float)*m_iKernelSize*m_iKernelSize);
    m_fImagGaborFilter = (float *) CPLMalloc(sizeof(float)*m_iKernelSize*m_iKernelSize);
    // gray only
    GDALRasterBand * poBand = poDataset->GetRasterBand(1);
    poBand->RasterIO( GF_Read, 0, 0, m_iXSize, m_iYSize, m_fOrgArray, m_iXSize, m_iYSize, GDT_Float32, 0, 0 );

    KUtility::reflectExtend<float>(m_fOrgArray,m_fExtArray,m_iXSize,m_iYSize,m_iKernelRadius);

    m_extImageWidth = m_iXSize + 2*m_iKernelRadius;

    GDALClose(piDataset);
    GDALClose(poDataset);
}

KGabor::~KGabor()
{
    CPLFree(m_fOrgArray);
    CPLFree(m_fExtArray);
    CPLFree(m_fGaborFilter);
    CPLFree(m_fImagGaborFilter);
}

void KGabor::build()
{
    int totalPixels = m_iXSize*m_iYSize;
    int totalScale = 5;
    float * pResultArray = (float *) CPLMalloc(sizeof(float)*totalPixels);

    KProgressBar progressBar("calcGabor",totalScale*m_iMaxDegree*totalPixels,80);
    K_PROGRESS_START(progressBar);

    for(int scale = 0;scale < totalScale;++scale){
        for(int dir = 0;dir < m_iMaxDegree;++dir){
            getGaborFilter(scale,dir);
            int desY = 0;
            for(int orgY=m_iKernelRadius;orgY<m_iYSize+m_iKernelRadius;++orgY,++desY){
                int desX = 0;
                for(int orgX=m_iKernelRadius;orgX<m_iXSize+m_iKernelRadius;++orgX,++desX){
                    float imag=0.;
                    float real=0.;
                    int tempOrgY=orgY-m_iKernelRadius;
                    // begin convolution
                    for(int iY=0;iY<m_iKernelSize;++iY,++tempOrgY){
                        int tempOrgX=orgX-m_iKernelRadius;
                        for(int iX=0;iX<m_iKernelSize;++iX,++tempOrgX){
                            imag+=m_fExtArray[tempOrgY*m_extImageWidth+tempOrgX]*m_fGaborFilter[iY*m_iKernelSize+iX];
                            real+=m_fExtArray[tempOrgY*m_extImageWidth+tempOrgX]*m_fImagGaborFilter[iY*m_iKernelSize+iX];
                        }
                    }
                    // calculate energy
                    pResultArray[desY*m_iXSize+desX]=imag*imag+real*real;

                    progressBar.autoUpdate();
                }
            }
            // printImage(pResultArray,QString("%1_%2_%3.tif").arg(m_sOutputRoot+"garbored").arg(scale).arg(dir));
            // normarlize
            // downsample
            // block
        }
    }

    K_PROGRESS_END(progressBar);
    CPLFree(pResultArray);

//    int scales=5;
//    int directions=8;
//    GDALDriver *m_poDriver = GetGDALDriverManager()->GetDriverByName("BMP");
//    QString tempName("C://real.bmp");
//    GDALDataset *pGaborDataset = m_poDriver->Create(tempName.toUtf8().data(),directions*m_iKernelSize,scales*m_iKernelSize
//                                 ,1,GDT_Byte,0);


//    getGaborFilter(0,0,pGaborDataset,true);
//    getGaborFilter(0,1,pGaborDataset,true);
//    getGaborFilter(0,2,pGaborDataset,true);
//    getGaborFilter(0,3,pGaborDataset,true);
//    getGaborFilter(0,4,pGaborDataset,true);
//    getGaborFilter(0,5,pGaborDataset,true);
//    getGaborFilter(0,6,pGaborDataset,true);
//    getGaborFilter(0,7,pGaborDataset,true);

//    getGaborFilter(1,0,pGaborDataset,true);
//    getGaborFilter(1,1,pGaborDataset,true);
//    getGaborFilter(1,2,pGaborDataset,true);
//    getGaborFilter(1,3,pGaborDataset,true);
//    getGaborFilter(1,4,pGaborDataset,true);
//    getGaborFilter(1,5,pGaborDataset,true);
//    getGaborFilter(1,6,pGaborDataset,true);
//    getGaborFilter(1,7,pGaborDataset,true);

//    getGaborFilter(2,0,pGaborDataset,true);
//    getGaborFilter(2,1,pGaborDataset,true);
//    getGaborFilter(2,2,pGaborDataset,true);
//    getGaborFilter(2,3,pGaborDataset,true);
//    getGaborFilter(2,4,pGaborDataset,true);
//    getGaborFilter(2,5,pGaborDataset,true);
//    getGaborFilter(2,6,pGaborDataset,true);
//    getGaborFilter(2,7,pGaborDataset,true);

//    getGaborFilter(3,0,pGaborDataset,true);
//    getGaborFilter(3,1,pGaborDataset,true);
//    getGaborFilter(3,2,pGaborDataset,true);
//    getGaborFilter(3,3,pGaborDataset,true);
//    getGaborFilter(3,4,pGaborDataset,true);
//    getGaborFilter(3,5,pGaborDataset,true);
//    getGaborFilter(3,6,pGaborDataset,true);
//    getGaborFilter(3,7,pGaborDataset,true);

//    getGaborFilter(4,0,pGaborDataset,true);
//    getGaborFilter(4,1,pGaborDataset,true);
//    getGaborFilter(4,2,pGaborDataset,true);
//    getGaborFilter(4,3,pGaborDataset,true);
//    getGaborFilter(4,4,pGaborDataset,true);
//    getGaborFilter(4,5,pGaborDataset,true);
//    getGaborFilter(4,6,pGaborDataset,true);
//    getGaborFilter(4,7,pGaborDataset,true);

//    GDALClose(pGaborDataset);

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

void KGabor::getGaborFilter(int scale, int dir, GDALDataset *dataset, bool bePrint)
{
    float Kmax=M_PI/2.;
    float Delta=M_PI*2.;
    float f=std::pow(2,0.5);
    float Kv=Kmax/(std::pow(f,scale));
    float FaiMiu=dir*M_PI/m_iMaxDegree;
    float Kuv_real=Kv*std::cos(FaiMiu);
    float Kuv_imag=Kv*std::sin(FaiMiu);
    for(int index = 0;index<m_iKernelSize*m_iKernelSize;++index){
        m_fGaborFilter[index]=0.;
        m_fImagGaborFilter[index]=0.;
    }

    for(int iY=-m_iKernelRadius;iY<m_iKernelRadius+1;++iY){
        for(int iX=-m_iKernelRadius;iX<m_iKernelRadius+1;++iX){
            float temp = (Kuv_real*Kuv_real+Kuv_imag*Kuv_imag);
            temp = temp/(Delta*Delta)*std::exp(-temp*(iY*iY+iX*iX)/(2*Delta*Delta));
            m_fGaborFilter[m_iKernelSize*(iY+m_iKernelRadius)+iX+m_iKernelRadius]=temp*(std::cos(Kuv_real*iX+Kuv_imag*iY)-std::exp(-Delta*Delta/2));
            m_fImagGaborFilter[m_iKernelSize*(iY+m_iKernelRadius)+iX+m_iKernelRadius]=temp*(std::sin(Kuv_real*iX+Kuv_imag*iY));
        }
    }
    if(bePrint){
        printFilter(m_fGaborFilter,m_sOutputRoot+QString("output-gabor-real(scale_%1-dir_%2).bmp").arg(scale).arg(dir));
        printFilter(m_fImagGaborFilter,/*"C:/tempGabor/imag/"*/m_sOutputRoot+QString("output-gabor-imag(scale_%1-dir_%2).bmp").arg(scale).arg(dir));
        printFilter(dataset,m_fGaborFilter,dir,scale);
    }
}

void KGabor::printFilter(float *filter, QString name)
{
    float min=(std::numeric_limits<float>::max)();
    float max=(std::numeric_limits<float>::min)();
    float *tempFilter = (float *) CPLMalloc(sizeof(float)*m_iKernelSize*m_iKernelSize);

    for(int index=0;index<m_iKernelSize*m_iKernelSize;++index){
        if(filter[index]<min) min=filter[index];
        if(filter[index]>max) max=filter[index];
    }
    for(int index=0;index<m_iKernelSize*m_iKernelSize;++index){
        tempFilter[index]=(filter[index]-min)*255./(max-min);
    }
    GDALDriver *m_poDriver = GetGDALDriverManager()->GetDriverByName("BMP");
    GDALDataset *poDataset = m_poDriver->Create(name.toUtf8().data(),m_iKernelSize,m_iKernelSize
                                 ,1,GDT_Byte,0);

    GDALRasterBand * poBand = poDataset->GetRasterBand(1);

    poBand->RasterIO( GF_Write, 0, 0, m_iKernelSize, m_iKernelSize, tempFilter, m_iKernelSize, m_iKernelSize, GDT_Float32, 0, 0 );
    poBand->FlushCache();
    poDataset->FlushCache();
    GDALClose(poDataset);
    CPLFree(tempFilter);
}

void KGabor::printFilter(GDALDataset *image,float *filter, int start, int end)
{
    float min=(std::numeric_limits<float>::max)();
    float max=(std::numeric_limits<float>::min)();
    float *tempFilter = (float *) CPLMalloc(sizeof(float)*m_iKernelSize*m_iKernelSize);

    for(int index=0;index<m_iKernelSize*m_iKernelSize;++index){
        if(filter[index]<min) min=filter[index];
        if(filter[index]>max) max=filter[index];
    }
    for(int index=0;index<m_iKernelSize*m_iKernelSize;++index){
        tempFilter[index]=(filter[index]-min)*255./(max-min);
    }
    GDALRasterBand * poBand = image->GetRasterBand(1);

    poBand->RasterIO( GF_Write, start*m_iKernelSize, end*m_iKernelSize, m_iKernelSize, m_iKernelSize, tempFilter, m_iKernelSize, m_iKernelSize, GDT_Float32, 0, 0 );
    poBand->FlushCache();
    image->FlushCache();
    CPLFree(tempFilter);
}

void KGabor::printImage(float *image, QString name)
{
    GDALDriver *m_poDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset *poDataset = m_poDriver->Create(name.toUtf8().data(),m_iXSize,m_iYSize
                                 ,1,GDT_Float32,0);

    GDALRasterBand * poBand = poDataset->GetRasterBand(1);

    poBand->RasterIO( GF_Write, 0, 0, m_iXSize, m_iYSize, image, m_iXSize, m_iYSize, GDT_Float32, 0, 0 );
    poBand->FlushCache();
    poDataset->FlushCache();
    GDALClose(poDataset);
}














