#include "ksplitimage.h"

#include "kpicinfo.h"
#include "kprogressbar.h"

#include <QRegularExpression>
#include <QStringList>
#include <QDir>

KSplitImage::KSplitImage(GDALDataset *piDataSet, QString name)
     :m_piDataSet(piDataSet),
      m_sPreFileName(""),
      m_sAftFileName("")
{
    if(KPicInfo::dataAttach(m_piDataSet)) { KPicInfo::getInstance()->build();}

    QString tempName = KPicInfo::getInstance()->getFileNoExtName();

    QRegularExpression re("[\\/\\\\//]");

    if(!name.isEmpty()){
        int index = tempName.lastIndexOf(re);
        for(;index<tempName.length();++index){
            if(tempName.at(index)==92 || tempName.at(index)==47) continue;
            else break;
        }
        tempName=tempName.left(index);
        tempName += name;
    }

    m_sPreFileName=tempName;

    const char *pszFormat = m_piDataSet->GetDriverName();
    m_pDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
    if( m_pDriver == NULL )
    {
        std::cout<<"KSplitImage:GetGDALDriverManager failed!"<<std::endl;
        exit( 1 );
    }
    if( CSLFetchBoolean( m_pDriver->GetMetadata(), GDAL_DCAP_CREATE, FALSE ) )
    {
        qDebug( "KSplitImage:Driver %s supports Create() method.", pszFormat );
        m_sAftFileName += KPicInfo::getInstance()->getFileExtName();
    }
    else
    {
        m_pDriver = GetGDALDriverManager()->GetDriverByName("BMP");
        m_sAftFileName += ".bmp";
    }
    if(name.contains('\\')||name.contains('/')){
        QStringList tempList = tempName.split(re);
        tempName.clear();
        for(int pos = 0;pos<tempList.length()-1;++pos)
        {
            tempName+=tempList[pos];
            tempName+="/";
        }
        QDir tempDir;
        tempDir.mkpath(tempName);
    }
}

GUInt16 KSplitImage::split(int width, int height)
{
    if(KPicInfo::dataAttach(m_piDataSet)) { KPicInfo::getInstance()->build();}
    int bandNum = KPicInfo::getInstance()->getBandNum();
    int nXSize = KPicInfo::getInstance()->getWidth();
    int nYSize = KPicInfo::getInstance()->getHeight();
    unsigned int nXSum = nXSize / width;
    unsigned int nYSum = nYSize / height;
    unsigned int nXDelta = nXSize - nXSum * width;
    unsigned int nYDelta = nYSize - nYSum * height;
    float *pafData = (float *) CPLMalloc(sizeof(float)*width*height);
    GDALRasterBand * piBand = NULL;
    GDALRasterBand * poBand = NULL;

    GUInt16 imgCnt = 0;

    KProgressBar progressBar("Spliting the Image",nXSum*nYSum*bandNum,80);
    K_PROGRESS_START(progressBar);
    QString temp=QString("%1").arg(nXSum*nYSum);
    int suffixLength = temp.length();
    for(unsigned int nYIndex(0),nYPos(nYDelta/2);nYIndex<nYSum;++nYIndex,nYPos+=height){
        for(unsigned int nXIndex(0),nXPos(nXDelta/2);nXIndex<nXSum;++nXIndex,nXPos+=width){
            imgCnt++;
            // build the file name
            QString tempString=QString("%1").arg(imgCnt);

            for(int i = tempString.length();i<suffixLength;++i)
            {
                tempString.insert(0,'0');
            }
            tempString = m_sPreFileName+tempString+m_sAftFileName;

            GDALDataset *poDataset = m_pDriver->Create(tempString.toUtf8().data(),width,height
                                         ,bandNum,KPicInfo::getInstance()->getType(),0);
            // copy image data one by one
            for(int bandIndex = 0;bandIndex < bandNum;++bandIndex)
            {
                piBand = m_piDataSet->GetRasterBand(bandIndex + 1);

                piBand->RasterIO( GF_Read, nXPos, nYPos, width, height, pafData, width, height, GDT_Float32, 0, 0 );

                poBand = poDataset->GetRasterBand(bandIndex + 1);
                poBand->RasterIO( GF_Write, 0, 0, width, height, pafData, width, height, GDT_Float32, 0, 0 );
                poBand->FlushCache();

                progressBar.autoUpdate();
            }
            poDataset->FlushCache();
            GDALClose(poDataset);
        }
    }

    K_PROGRESS_END(progressBar);

    CPLFree(pafData);

    return nXSum*nYSum;
}

