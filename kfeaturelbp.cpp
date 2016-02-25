#include "kfeaturelbp.h"
#include "kpicinfo.h"
#include "common.h"

#include "cpl_conv.h" // for CPLMalloc()

#include <boost/dynamic_bitset.hpp>
#include <boost/math/special_functions/sin_pi.hpp>
#include <boost/math/special_functions/cos_pi.hpp>

#include <QCoreApplication>
#include <QFile>
#include <QDebug>

KFeatureLBP::~KFeatureLBP()
{
    char ** filelist =m_piDataset->GetFileList();
    GDALClose(m_piDataset);
    // just fetch the first one
    QString temp(*filelist);
    CSLDestroy (filelist);
    QFile file(temp);
    if(!file.remove()) std::cout<<"KFeatureLBP:remove the temp file failed!"<<std::endl;
    if(NULL != refTable) CPLFree(refTable);
}

KFeatureLBP::KFeatureLBP(GDALDataset *piDataset, GDALDataset *poDataset, int sampleNum, int kernelRadius)
    : m_piDataset(piDataset),
      m_poDataset(poDataset),
      m_kernelRadius(kernelRadius),
      m_sampleNum(sampleNum),
      m_fileName(""),
      refTable(NULL)
{

}

bool KFeatureLBP::calLBP(float *inBuff, GByte *outBuff, int width, int height,bool toBeNormarlized, KProgressBar * pProgressBar)
{
    assert(NULL != outBuff);
    assert(NULL != inBuff);

    boost::dynamic_bitset<> tempOne(m_sampleNum,1);
    boost::dynamic_bitset<> tempZero(m_sampleNum,0);

    for(int iYPos = m_kernelRadius,iYDes=0;iYPos<height-m_kernelRadius;++iYPos,++iYDes)
    {
        for(int iXPos = m_kernelRadius,iXDes=0;iXPos<width-m_kernelRadius;++iXPos,++iXDes)
        {
            boost::dynamic_bitset<> tempbin(m_sampleNum,0);
            // calculate anticlockwise
            for(int nsCnt=0;nsCnt<m_sampleNum;++nsCnt)
            {
                float fxAxis = iXPos + m_kernelRadius*boost::math::cos_pi(2.*nsCnt/m_sampleNum);
                float fyAxis = iYPos - m_kernelRadius*boost::math::sin_pi(2.*nsCnt/m_sampleNum);
                int ixAxis = static_cast<int>(fxAxis);
                int iyAxis = static_cast<int>(fyAxis);
                fxAxis = fxAxis-ixAxis;
                fyAxis = fyAxis-iyAxis;
                /** bilinear interpolation algorithm
                 * y x-------
                 * |(0,0)
                 * |  |a   b|
                 * |  | x,y | --> gray(x,y)=(1-x)*[(1-y)*a + y*c] + x*[(1-y)*b + y*d]
                 * |  |c   d|
                 *
                 *                  |f(0,0) f(1,0)||1-y|
                 * gray(x,y)=[1-x,x]|             ||   |
                 *                  |f(0,1) f(1,1)|| y |
                 */
                float tempValue = (1-fxAxis)*((1-fyAxis)*inBuff[iyAxis*width+ixAxis]
                        +fyAxis*inBuff[(iyAxis+1)*width+ixAxis])
                        +fxAxis*((1-fyAxis)*inBuff[iyAxis*width+ixAxis+1]
                        +fyAxis*inBuff[(iyAxis+1)*width+ixAxis+1]);

                tempbin <<= 1;
                tempbin |= tempValue>inBuff[iYPos*width+iXPos]?tempOne:tempZero;
            }
            if(NULL != pProgressBar) pProgressBar->autoUpdate();
            if(toBeNormarlized==true)
                outBuff[iYDes*(width-2*m_kernelRadius)+iXDes]=refTable[tempbin.to_ulong()]*256./(m_sampleNum+2.);
            else
                outBuff[iYDes*(width-2*m_kernelRadius)+iXDes]=refTable[tempbin.to_ulong()];
//            tempbin=tempZero;
//            tempbin <<= 1;
//            tempbin |= inBuff[iYPos*width+iXPos+1]>inBuff[iYPos*width+iXPos]?tempOne:tempZero;
//            tempbin <<= 1;
//            tempbin |= inBuff[(iYPos-1)*width+iXPos+1]>inBuff[iYPos*width+iXPos]?tempOne:tempZero;
//            tempbin <<= 1;
//            tempbin |= inBuff[(iYPos-1)*width+iXPos]>inBuff[iYPos*width+iXPos]?tempOne:tempZero;
//            tempbin <<= 1;
//            tempbin |= inBuff[(iYPos-1)*width+iXPos-1]>inBuff[iYPos*width+iXPos]?tempOne:tempZero;
//            tempbin <<= 1;
//            tempbin |= inBuff[iYPos*width+iXPos-1]>inBuff[iYPos*width+iXPos]?tempOne:tempZero;
//            tempbin <<= 1;
//            tempbin |= inBuff[(iYPos+1)*width+iXPos-1]>inBuff[iYPos*width+iXPos]?tempOne:tempZero;
//            tempbin <<= 1;
//            tempbin |= inBuff[(iYPos+1)*width+iXPos]>inBuff[iYPos*width+iXPos]?tempOne:tempZero;
//            tempbin <<= 1;
//            tempbin |= inBuff[(iYPos+1)*width+iXPos+1]>inBuff[iYPos*width+iXPos]?tempOne:tempZero;
//            outBuff[iYDes*(width-2*m_kernelRadius)+iXDes]=tempbin.to_ulong();
        }
    }

    return true;
}

bool KFeatureLBP::run(Kapok::K_BorderTypes type,bool toBeNormarlized)
{
    int bandNum = KPicInfo::getInstance()->getBandNum();
    int nXSize = KPicInfo::getInstance()->getWidth();
    int nYSize = KPicInfo::getInstance()->getHeight();

    GDALRasterBand * piBand = NULL;
    GDALRasterBand * poBand = NULL;
    float *pafData = NULL;
    GByte *pafOutData = NULL;
    int err_code = 1 + bandNum;
    if(externDataSet(type)) err_code -= 1;

    // create the reference table
    GUIntBig maxIndex = get2Power(m_sampleNum);
    GUIntBig minValue = 0;
    KProgressBar progressBar1("Calculating LBP refTable",maxIndex,80);
    K_PROGRESS_START(progressBar1);
    for(GUIntBig index = 0;index < maxIndex;++index)
    {
        // Achieving Rotation Invariance
        minValue = index;
        // leave the template args to be empty means determined by the compiler
        boost::dynamic_bitset<> bin(m_sampleNum, index);
        for(int pos = 0;pos<m_sampleNum;++pos){
            boost::dynamic_bitset<> tempbin=(bin>>pos)|(bin<<m_sampleNum-pos);
            if(minValue>tempbin.to_ulong()){
                minValue = tempbin.to_ulong();
            }
        }
        // confirm the "Uniform" Patterns
        boost::dynamic_bitset<> temp(m_sampleNum,minValue);
        GByte cycleSum = abs(temp[0] - temp[m_sampleNum - 1]);
        // the default value is set to be the num of bits which are set
        refTable[index] = bin.count();
        for(GByte pos = 0;pos < m_sampleNum - 1;++pos)
        {
            cycleSum += abs(temp[pos]-temp[pos+1]);
            if(cycleSum > 2){
                refTable[index] = m_sampleNum+1;
                break;
            }
        }
        progressBar1.autoUpdate();
    }
    K_PROGRESS_END(progressBar1);

    KProgressBar progressBar2("Calculating LBP feature",bandNum*nXSize*nYSize,80);
    K_PROGRESS_START(progressBar2);
    // get the LBP feature
    pafOutData = (GByte *) CPLMalloc(sizeof(float)*nXSize*nYSize);
    pafData = (float *) CPLMalloc(sizeof(float)*(nXSize + 2*m_kernelRadius + 1)*(nYSize + 2*m_kernelRadius + 1));
    for(int index = 0; index < bandNum; ++index)
    {
        piBand = m_piDataset->GetRasterBand(index + 1);
        poBand = m_poDataset->GetRasterBand(index + 1);

        piBand->RasterIO( GF_Read, 0, 0, nXSize + 2*m_kernelRadius + 1, nYSize + 2*m_kernelRadius + 1
                          , pafData, nXSize + 2*m_kernelRadius + 1, nYSize + 2*m_kernelRadius + 1
                          , GDT_Float32, 0, 0 );

        if(calLBP(pafData,pafOutData,nXSize + 2*m_kernelRadius,nYSize + 2*m_kernelRadius,toBeNormarlized,&progressBar2)) err_code -= 1;

        poBand->RasterIO( GF_Write, 0, 0, nXSize , nYSize, pafOutData
                          , nXSize, nYSize, GDT_Byte, 0, 0 );
        poBand->FlushCache();

    }
    K_PROGRESS_END(progressBar2);

    GDALClose(m_poDataset);

    CPLFree(pafData);
    CPLFree(pafOutData);

    return (0 == err_code);
}

GDALDataset *KFeatureLBP::build(QString fileName)
{
    m_fileName = fileName;
    if(KPicInfo::dataAttach(m_piDataset)) KPicInfo::getInstance()->build();

    int bandNum = KPicInfo::getInstance()->getBandNum();
    int nXSize = KPicInfo::getInstance()->getWidth();
    int nYSize = KPicInfo::getInstance()->getHeight();

    if(m_sampleNum > 24){
        std::cout<<"KFeatureLBP:Sample points cannot more than 24!"<<std::endl;
        exit( 1 );
    }

    bool beSame = K_CheckDataSetEqu(m_piDataset,m_poDataset);

    if(!beSame){

        QString tempName=QCoreApplication::applicationDirPath()+"/tempImg%%KFeatureLBP";
        QString tempInputName=QCoreApplication::applicationDirPath()+"/tempExtImg%%KFeatureLBP."+KPicInfo::getInstance()->getFileExtName();;
        if(!m_fileName.isEmpty()){ tempName = m_fileName; }

        const char *pszFormat = m_piDataset->GetDriverName();
        GDALDriver *poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
        if( poDriver == NULL )
        {
            std::cout<<"KFeatureLBP:GetGDALDriverManager failed!"<<std::endl;
            exit( 1 );
        }
        if( CSLFetchBoolean( poDriver->GetMetadata(), GDAL_DCAP_CREATE, FALSE ) )
        {
            qDebug( "KFeatureLBP:Driver %s supports Create() method.", pszFormat );
            tempName += KPicInfo::getInstance()->getFileExtName();
        }
        else
        {
            poDriver = GetGDALDriverManager()->GetDriverByName("BMP");
            tempName += ".bmp";
        }
        if(NULL != m_poDataset) GDALClose(m_poDataset);
        // allocate the output Dataset
        m_poDataset = poDriver->Create(tempName.toUtf8().data(),nXSize,nYSize
                                       ,bandNum,GDT_Byte,0);
        // store the previous handle
        m_piOrgDataset = m_piDataset;
        // +1 is to guarantee the success of bilinear interpolation algorithm
        // build the new handle to store the extended image
        m_piDataset = poDriver->Create(tempInputName.toUtf8().data()
                                       ,nXSize+2*m_kernelRadius + 1,nYSize+2*m_kernelRadius + 1
                                       ,bandNum,KPicInfo::getInstance()->getType(),0);

    }else{
        std::cout<<"KFeatureLBP:the input and output cannot be same!"<<std::endl;
        m_poDataset = NULL;
    }

    assert(NULL != m_poDataset);

    if(NULL != refTable) CPLFree(refTable);
    refTable = (unsigned char *) CPLMalloc(sizeof(unsigned char)*get2Power(m_sampleNum));

    return m_poDataset;
}

bool KFeatureLBP::constExtend(float * inBuff, float * outBuff, int width, int height, float defaultValue)
{
    assert(NULL != outBuff);
    assert(NULL != inBuff);

    // fill the outbuff with the default value
    for(GIntBig index = 0;index < (width + 1) * (height + 1);++index)
    {
        outBuff[index] = defaultValue;
    }
    // copy the source to the outbuff
    for(int iYDes = m_kernelRadius,iYSrc = 0;iYDes < height - m_kernelRadius;++iYDes,++iYSrc)
    {
        for(int iXDes = m_kernelRadius,iXSrc = 0;iXDes < width - m_kernelRadius;++iXDes,++iXSrc)
        {
            outBuff[iYDes*width+iXDes] = inBuff[iYSrc*(width-2*m_kernelRadius)+iXSrc];
        }
    }
    return true;
}

bool KFeatureLBP::reflectExtend(float * inBuff, float * outBuff, int width, int height)
{
    assert(NULL != outBuff);
    assert(NULL != inBuff);

    if(width < 3 * m_kernelRadius || height < 3 * m_kernelRadius)
    {
        std::cout<<"KFeatureLBP:each line of the image cannot be shorter than KernelSize!"<<std::endl;
        exit( 1 );
    }
    // copy the source to the outbuff
    for(int iYDes = m_kernelRadius,iYSrc = 0;iYDes < height - m_kernelRadius;++iYDes,++iYSrc)
    {
        for(int iXDes = m_kernelRadius,iXSrc = 0;iXDes < width - m_kernelRadius;++iXDes,++iXSrc)
        {
            outBuff[iYDes*width+iXDes] = inBuff[iYSrc*(width-2*m_kernelRadius)+iXSrc];
        }
    }

    // fill horizontal
    for(int iYDes = m_kernelRadius;iYDes < height - m_kernelRadius;++iYDes)
    {
        // fill the head of each line
        int iXDes = m_kernelRadius-1,iXSrc = m_kernelRadius+1;
        for(;iXDes >= 0 && iXSrc < width - m_kernelRadius;--iXDes,++iXSrc)
        {
            outBuff[iYDes*width+iXDes] = outBuff[iYDes*width+iXSrc];
        }
        for(;iXDes >= 0;--iXDes)
        {
            outBuff[iYDes*width+iXDes] = outBuff[iYDes*width+iXSrc-1];
        }
        // fill the end of each line
        iXDes = width - m_kernelRadius,iXSrc = width - m_kernelRadius - 2;
        for(;iXDes < width + 1&& iXSrc >= m_kernelRadius;++iXDes,--iXSrc)
        {
            outBuff[iYDes*width+iXDes] = outBuff[iYDes*width+iXSrc];
        }
        for(;iXDes < width + 1;++iXDes)
        {
            outBuff[iYDes*width+iXDes] = outBuff[iYDes*width+iXSrc+1];
        }
    }

    // fill vertical -- think rotate 90 degrees
    for(int iYDes = m_kernelRadius;iYDes < width - m_kernelRadius;++iYDes)
    {
        // fill the head of each line
        int iXDes = m_kernelRadius-1,iXSrc = m_kernelRadius+1;
        for(;iXDes >= 0 && iXSrc < height - m_kernelRadius;--iXDes,++iXSrc)
        {
            outBuff[iXDes*width+iYDes] = outBuff[iXSrc*width+iYDes];
        }
        for(;iXDes >= 0;--iXDes)
        {
            outBuff[iXDes*width+iYDes] = outBuff[(iXSrc-1)*width+iYDes];
        }
        // fill the end of each line
        iXDes = height - m_kernelRadius,iXSrc = height - m_kernelRadius - 2;
        for(;iXDes < height + 1 && iXSrc >= m_kernelRadius;++iXDes,--iXSrc)
        {
            outBuff[iXDes*width+iYDes] = outBuff[iXSrc*width+iYDes];
        }
        for(;iXDes < height + 1;++iXDes)
        {
            outBuff[iXDes*width+iYDes] = outBuff[(iXSrc+1)*width+iYDes];
        }
    }

    // fill four corners, please find the axis of symmetry carefully
    for(int iYDes = 0;iYDes < m_kernelRadius;++iYDes)
    {
        for(int iXDes = 0;iXDes < m_kernelRadius;++iXDes)
        {
            outBuff[iYDes*width+iXDes] = (outBuff[(2*m_kernelRadius-iYDes)*width+iXDes]+outBuff[iYDes*width+2*m_kernelRadius-iXDes])/2;
        }
    }
    for(int iYDes = 0;iYDes < m_kernelRadius;++iYDes)
    {
        for(int iXDes = width-m_kernelRadius;iXDes < width + 1;++iXDes)
        {
            outBuff[iYDes*width+iXDes] = (outBuff[(2*m_kernelRadius-iYDes)*width+iXDes]+outBuff[iYDes*width+2*(width-m_kernelRadius-1)-iXDes])/2;
        }
    }
    for(int iYDes = height - m_kernelRadius;iYDes < height + 1;++iYDes)
    {
        for(int iXDes = 0;iXDes < m_kernelRadius;++iXDes)
        {
            outBuff[iYDes*width+iXDes] = (outBuff[(2*(height-m_kernelRadius-1)-iYDes)*width+iXDes]+outBuff[iYDes*width+2*m_kernelRadius-iXDes])/2;
        }
    }
    for(int iYDes = height - m_kernelRadius;iYDes < height + 1;++iYDes)
    {
        for(int iXDes = width - m_kernelRadius;iXDes < width + 1;++iXDes)
        {
            outBuff[iYDes*width+iXDes] = (outBuff[(2*(height-m_kernelRadius-1)-iYDes)*width+iXDes]+outBuff[iYDes*width+2*(width-m_kernelRadius-1)-iXDes])/2;
        }
    }
    return true;
}

bool KFeatureLBP::replicateExtend(float * inBuff, float * outBuff, int width, int height)
{
    assert(NULL != outBuff);
    assert(NULL != inBuff);

    if(width < 3 * m_kernelRadius || height < 3 * m_kernelRadius)
    {
        std::cout<<"KFeatureLBP:each line of the image cannot be shorter than KernelSize!"<<std::endl;
        exit( 1 );
    }
    // copy the source to the outbuff
    for(int iYDes = m_kernelRadius,iYSrc = 0;iYDes < height - m_kernelRadius;++iYDes,++iYSrc)
    {
        for(int iXDes = m_kernelRadius,iXSrc = 0;iXDes < width - m_kernelRadius;++iXDes,++iXSrc)
        {
            outBuff[iYDes*width+iXDes] = inBuff[iYSrc*(width-2*m_kernelRadius)+iXSrc];
        }
    }

    // fill horizontal
    for(int iYDes = m_kernelRadius;iYDes < height - m_kernelRadius;++iYDes)
    {
        // fill the head of each line
        int iXDes = m_kernelRadius-1,iXSrc = m_kernelRadius;
        for(;iXDes >= 0 && iXSrc < width - m_kernelRadius;--iXDes)
        {
            outBuff[iYDes*width+iXDes] = outBuff[iYDes*width+iXSrc];
        }
        for(;iXDes >= 0;--iXDes)
        {
            outBuff[iYDes*width+iXDes] = outBuff[iYDes*width+iXSrc];
        }
        // fill the end of each line
        iXDes = width - m_kernelRadius,iXSrc = width - m_kernelRadius - 1;
        for(;iXDes < width + 1 && iXSrc >= m_kernelRadius;++iXDes)
        {
            outBuff[iYDes*width+iXDes] = outBuff[iYDes*width+iXSrc];
        }
        for(;iXDes < width + 1;++iXDes)
        {
            outBuff[iYDes*width+iXDes] = outBuff[iYDes*width+iXSrc];
        }
    }

    // fill vertical -- think rotate 90 degrees
    for(int iYDes = m_kernelRadius;iYDes < width - m_kernelRadius;++iYDes)
    {
        // fill the head of each line
        int iXDes = m_kernelRadius-1,iXSrc = m_kernelRadius;
        for(;iXDes >= 0 && iXSrc < height - m_kernelRadius;--iXDes)
        {
            outBuff[iXDes*width+iYDes] = outBuff[iXSrc*width+iYDes];
        }
        for(;iXDes >= 0;--iXDes)
        {
            outBuff[iXDes*width+iYDes] = outBuff[iXSrc*width+iYDes];
        }
        // fill the end of each line
        iXDes = height - m_kernelRadius,iXSrc = height - m_kernelRadius - 1;
        for(;iXDes < height + 1 && iXSrc >= m_kernelRadius;++iXDes)
        {
            outBuff[iXDes*width+iYDes] = outBuff[iXSrc*width+iYDes];
        }
        for(;iXDes < height + 1;++iXDes)
        {
            outBuff[iXDes*width+iYDes] = outBuff[iXSrc*width+iYDes];
        }
    }

    // fill four corners, please find the axis of symmetry carefully
    for(int iYDes = 0;iYDes < m_kernelRadius;++iYDes)
    {
        for(int iXDes = 0;iXDes < m_kernelRadius;++iXDes)
        {
            outBuff[iYDes*width+iXDes] = outBuff[m_kernelRadius*width+m_kernelRadius];
        }
    }
    for(int iYDes = 0;iYDes < m_kernelRadius;++iYDes)
    {
        for(int iXDes = width-m_kernelRadius;iXDes < width + 1;++iXDes)
        {
            outBuff[iYDes*width+iXDes] = outBuff[m_kernelRadius*width+width-m_kernelRadius-1];
        }
    }
    for(int iYDes = height - m_kernelRadius;iYDes < height + 1;++iYDes)
    {
        for(int iXDes = 0;iXDes < m_kernelRadius;++iXDes)
        {
            outBuff[iYDes*width+iXDes] = outBuff[(height-m_kernelRadius-1)*width+m_kernelRadius];
        }
    }
    for(int iYDes = height - m_kernelRadius;iYDes < height + 1;++iYDes)
    {
        for(int iXDes = width - m_kernelRadius;iXDes < width + 1;++iXDes)
        {
            outBuff[iYDes*width+iXDes] = outBuff[(height-m_kernelRadius-1)*width+width-m_kernelRadius-1];
        }
    }
    return true;
}

bool KFeatureLBP::externDataSet(Kapok::K_BorderTypes type,float defaultValue)
{
    int bandNum = KPicInfo::getInstance()->getBandNum();
    int nXSize = KPicInfo::getInstance()->getWidth();
    int nYSize = KPicInfo::getInstance()->getHeight();
    unsigned int err = bandNum;
    GDALRasterBand * piBand = NULL;
    GDALRasterBand * poBand = NULL;
    float *pafData = NULL;
    float *pafOutData = NULL;
    pafData = (float *) CPLMalloc(sizeof(float)*nXSize*nYSize);
    pafOutData = (float *) CPLMalloc(sizeof(float)*(nXSize + 2*m_kernelRadius + 1)*(nYSize + 2*m_kernelRadius + 1));
    for(int index = 0; index < bandNum; ++index)
    {
        piBand = m_piOrgDataset->GetRasterBand(index + 1);
        poBand = m_piDataset->GetRasterBand(index + 1);

        piBand->RasterIO( GF_Read, 0, 0, nXSize, nYSize, pafData, nXSize, nYSize, GDT_Float32, 0, 0 );
        switch(type)
        {
        case Kapok::Border_Constant:
            err -= constExtend(pafData, pafOutData, nXSize + 2*m_kernelRadius, nYSize + 2*m_kernelRadius, defaultValue);
            break;
        case Kapok::Border_Replicate:
            err -= replicateExtend(pafData, pafOutData, nXSize + 2*m_kernelRadius, nYSize + 2*m_kernelRadius);
            break;
        case Kapok::Border_Reflect:
        default:
            err -= reflectExtend(pafData, pafOutData, nXSize + 2*m_kernelRadius, nYSize + 2*m_kernelRadius);
            break;
        }

        poBand->RasterIO( GF_Write, 0, 0, nXSize + 2*m_kernelRadius + 1
                          , nYSize + 2*m_kernelRadius + 1, pafOutData
                          , nXSize + 2*m_kernelRadius + 1, nYSize + 2*m_kernelRadius + 1, GDT_Float32, 0, 0 );
        poBand->FlushCache();
    }
    CPLFree(pafData);
    CPLFree(pafOutData);

    return !err;
}
