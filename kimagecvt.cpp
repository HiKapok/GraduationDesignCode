#include "kimagecvt.h"
#include "common.h"
#include "kpicinfo.h"

#include <Qdebug>
#include <QString>
#include <QCoreApplication>
#include <QRegularExpression>

KImageCvt::KImageCvt()
{
}

GDALDataset * KImageCvt::normalize(GDALDataset *piDataset, GDALDataset *poDataset, float dMin, float dMax, QString name,GDALDataType type)
{
//    if(!K_CheckDataSetEqu(piDataset,poDataset))
//    {

//        GDALDataset *poDstDS;
//        poDstDS = poDriver->CreateCopy( pszDstFilename, poSrcDS, FALSE, NULL, NULL, NULL );/* Once we're done, close properly the dataset */
//        if( poDstDS != NULL )    GDALClose( (GDALDatasetH) poDstDS );

//    }
    if(KPicInfo::dataAttach(piDataset)) KPicInfo::getInstance()->build();
    float vMax = static_cast<float> (KPicInfo::getInstance()->getMax());
    float vMin = static_cast<float> (KPicInfo::getInstance()->getMin());
    return addWeighted(piDataset,poDataset,(dMax-dMin)/(vMax-vMin),dMin - vMin*(dMax-dMin)/(vMax-vMin),name,type);
}

GDALDataset * KImageCvt::addWeighted(GDALDataset *piDataset, GDALDataset *poDataset, float scale, float offset, QString name,GDALDataType type)
{
    float *pafData;
    if(KPicInfo::dataAttach(piDataset)) KPicInfo::getInstance()->build();
    int bandNum = KPicInfo::getInstance()->getBandNum();
    int nXSize = KPicInfo::getInstance()->getWidth();
    int nYSize = KPicInfo::getInstance()->getHeight();
    pafData = (float *) CPLMalloc(sizeof(float)*nXSize*nYSize);
    GDALRasterBand * piBand = NULL;
    bool beSame = K_CheckDataSetEqu(piDataset,poDataset);

    if(!beSame){
        QString tempName=QCoreApplication::applicationDirPath()+"/tempImg%%addWeighted";
        if(!name.isEmpty()){ tempName = name; }
        tempName += ".";

        const char *pszFormat = piDataset->GetDriverName();
        GDALDriver *poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
        if( poDriver == NULL )
        {
            std::cout<<"addWeighted:GetGDALDriverManager failed!"<<std::endl;
            exit( 1 );
        }
        if( CSLFetchBoolean( poDriver->GetMetadata(), GDAL_DCAP_CREATE, FALSE ) )
        {
            //qDebug( "addWeighted:Driver %s supports Create() method.", pszFormat );
            tempName += KPicInfo::getInstance()->getFileExtName();
        }
        else
        {
            poDriver = GetGDALDriverManager()->GetDriverByName("BMP");
            tempName += "bmp";
        }

        if(type == GDT_TypeCount) type=KPicInfo::getInstance()->getType();

        poDataset = poDriver->Create(tempName.toUtf8().data(),nXSize,nYSize
                                     ,bandNum,type,0);
        if(NULL == poDataset)
        {
            CPLFree(pafData);
            return NULL;
        }
    }else{
        qDebug()<<"addWeighted:the input and output are same!";
    }

    for(int bandIndex = 0;bandIndex < bandNum;++bandIndex)
    {
        piBand = piDataset->GetRasterBand(bandIndex + 1);

        piBand->RasterIO( GF_Read, 0, 0, nXSize, nYSize, pafData, nXSize, nYSize, GDT_Float32, 0, 0 );

        for(GIntBig index = 0;index < nXSize*nYSize;++index)
        {
            pafData[index] = scale * pafData[index] + offset;
        }

        if(beSame){
            piBand->RasterIO( GF_Write, 0, 0, nXSize, nYSize, pafData, nXSize, nYSize, GDT_Float32, 0, 0 );
            piBand->FlushCache();
        }else{
            GDALRasterBand * poBand = poDataset->GetRasterBand(bandIndex + 1);
            poBand->RasterIO( GF_Write, 0, 0, nXSize, nYSize, pafData, nXSize, nYSize, GDT_Float32, 0, 0 );
            poBand->FlushCache();
        }
    }
    poDataset->FlushCache();
    CPLFree(pafData);
    return poDataset;
}

GDALDataset * KImageCvt::colorReduce(GDALDataset *piDataset, GDALDataset *poDataset, int div, QString name,GDALDataType type)
{
    float *pafData;
    if(KPicInfo::dataAttach(piDataset)) { KPicInfo::getInstance()->build();}
    int bandNum = KPicInfo::getInstance()->getBandNum();
    int nXSize = KPicInfo::getInstance()->getWidth();
    int nYSize = KPicInfo::getInstance()->getHeight();
    pafData = (float *) CPLMalloc(sizeof(float)*nXSize*nYSize);
    GDALRasterBand * piBand = NULL;
    bool beSame = K_CheckDataSetEqu(piDataset,poDataset);

    if(!beSame){
        QString tempName=QCoreApplication::applicationDirPath()+"/tempImg%%colorReduce";
        if(!name.isEmpty()){ tempName = name; }

        const char *pszFormat = piDataset->GetDriverName();
        GDALDriver *poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
        if( poDriver == NULL )
        {
            std::cout<<"colorReduce:GetGDALDriverManager failed!"<<std::endl;
            exit( 1 );
        }
        if( CSLFetchBoolean( poDriver->GetMetadata(), GDAL_DCAP_CREATE, FALSE ) )
        {
            //qDebug( "colorReduce:Driver %s supports Create() method.", pszFormat );
            tempName += KPicInfo::getInstance()->getFileExtName();
        }
        else
        {
            poDriver = GetGDALDriverManager()->GetDriverByName("BMP");
            tempName += ".bmp";
        }

        if(type == GDT_TypeCount) type=KPicInfo::getInstance()->getType();
        poDataset = poDriver->Create(tempName.toUtf8().data(),nXSize,nYSize
                                     ,bandNum,type,0);
        if(NULL == poDataset)
        {
            CPLFree(pafData);
            return NULL;
        }
    }else{
        if(KPicInfo::getInstance()->getType()>5 || 0==KPicInfo::getInstance()->getType())
        {
            std::cout<<"*colorReduce:the input and output cannot be same when the input aren't integer!*"<<std::endl;
            exit( 1 );
        }else{
            qDebug()<<"colorReduce:the input and output are same!";
        }
    }

    for(int bandIndex = 0;bandIndex < bandNum;++bandIndex)
    {
        piBand = piDataset->GetRasterBand(bandIndex + 1);

        piBand->RasterIO( GF_Read, 0, 0, nXSize, nYSize, pafData, nXSize, nYSize, GDT_Float32, 0, 0 );

        for(GIntBig index = 0;index < nXSize*nYSize;++index)
        {
            pafData[index] = pafData[index] / div * div + div/2;
        }

        if(beSame){
            piBand->RasterIO( GF_Write, 0, 0, nXSize, nYSize, pafData, nXSize, nYSize, GDT_Float32, 0, 0 );
            piBand->FlushCache();
        }else{
            GDALRasterBand * poBand = poDataset->GetRasterBand(bandIndex + 1);
            poBand->RasterIO( GF_Write, 0, 0, nXSize, nYSize, pafData, nXSize, nYSize, GDT_Float32, 0, 0 );
            poBand->FlushCache();
        }
    }

    poDataset->FlushCache();
    CPLFree(pafData);
    return poDataset;
}

GDALDataset * KImageCvt::img2gray(GDALDataset *piDataset, GDALDataset *poDataset,QString name)
{
    if(KPicInfo::dataAttach(piDataset,true)) KPicInfo::getInstance()->build();
    int bandNum = KPicInfo::getInstance()->getBandNum();
    int nXSize = KPicInfo::getInstance()->getWidth();
    int nYSize = KPicInfo::getInstance()->getHeight();
    float vMax = static_cast<float> (KPicInfo::getInstance()->getMax());
    float vMin = static_cast<float> (KPicInfo::getInstance()->getMin());

    float scale = 255./(vMax-vMin);
    float offset = -vMin*255./(vMax-vMin);

    GDALRasterBand * piBand[3] = {0};

    QString tempName=QCoreApplication::applicationDirPath()+"/tempImg%%img2gray";
    if(!name.isEmpty()){ tempName = name; }

    // only deal with single band or RGB images
    if(1==bandNum) return addWeighted(piDataset,poDataset,scale,offset,tempName,GDT_Byte);
    else if(3 != bandNum) { std::cout<<"*just support single band or RGB images*"<<std::endl; return NULL; };

    float **pafData = new float *[3];
    pafData[0] = (float *) CPLMalloc(sizeof(float)*nXSize*nYSize);
    pafData[1] = (float *) CPLMalloc(sizeof(float)*nXSize*nYSize);
    pafData[2] = (float *) CPLMalloc(sizeof(float)*nXSize*nYSize);

    bool beSame = K_CheckDataSetEqu(piDataset,poDataset);

    if(!beSame){

        const char *pszFormat = piDataset->GetDriverName();
        GDALDriver *poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
        if( poDriver == NULL )
        {
            std::cout<<"img2gray:GetGDALDriverManager failed!"<<std::endl;
            exit( 1 );
        }
        if( CSLFetchBoolean( poDriver->GetMetadata(), GDAL_DCAP_CREATE, FALSE ) )
        {
            //qDebug( "img2gray:Driver %s supports Create() method.", pszFormat );
            tempName += KPicInfo::getInstance()->getFileExtName();
        }
        else
        {
            poDriver = GetGDALDriverManager()->GetDriverByName("BMP");
            tempName += ".bmp";
        }

        poDataset = poDriver->Create(tempName.toUtf8().data(),nXSize,nYSize
                                     ,1,GDT_Byte,0);
        if(NULL == poDataset)
        {
            CPLFree(pafData[0]);
            CPLFree(pafData[1]);
            CPLFree(pafData[2]);
            delete [] pafData;
            return NULL;
        }
    }else{
        CPLFree(pafData[0]);
        CPLFree(pafData[1]);
        CPLFree(pafData[2]);
        delete [] pafData;
        std::cout<<"img2gray:the input and output cannot be same!"<<std::endl;
        return NULL;
    }

    for(int bandIndex = 0;bandIndex < bandNum;++bandIndex)
    {
        piBand[bandIndex] = piDataset->GetRasterBand(bandIndex + 1);
        piBand[bandIndex]->RasterIO( GF_Read, 0, 0, nXSize, nYSize, pafData[bandIndex], nXSize, nYSize, GDT_Float32, 0, 0 );
        for(GIntBig index = 0;index < nXSize*nYSize;++index)
        {
            pafData[bandIndex][index] = scale * pafData[bandIndex][index] + offset;
        }
    }

    float *pafoData;
    pafoData = (float *) CPLMalloc(sizeof(float)*nXSize*nYSize);

    QStringList bandName = KPicInfo::getInstance()->getBandName();

    QRegularExpression re("Red");
    //qDebug()<<bandName.indexOf(re);
    float *pRedData = pafData[bandName.indexOf(re)];
    re.setPattern("Green");
    //qDebug()<<bandName.indexOf(re);
    float *pGreenData = pafData[bandName.indexOf(re)];
    re.setPattern("Blue");
    //qDebug()<<bandName.indexOf(re);
    float *pBlueData = pafData[bandName.indexOf(re)];

    for(GIntBig index = 0;index < nXSize*nYSize;++index)
    {
        pafoData[index] = pRedData[index]*0.299 + pGreenData[index]*0.587 + pBlueData[index]*0.114;
    }

    GDALRasterBand * poBand = poDataset->GetRasterBand(1);
    poBand->RasterIO( GF_Write, 0, 0, nXSize, nYSize, pafoData, nXSize, nYSize, GDT_Float32, 0, 0 );

    poBand->FlushCache();
    poDataset->FlushCache();
    CPLFree(pafData[0]);
    CPLFree(pafData[1]);
    CPLFree(pafData[2]);
    CPLFree(pafoData);
    delete [] pafData;
    return poDataset;
}

