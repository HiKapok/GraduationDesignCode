#include "kpicinfo.h"
#include "cpl_conv.h" // for CPLMalloc()

#include "common.h"

#include <QDebug>
#include <QStringList>

/** GDALDataType Tabel
GDT_Unknown  Unknown or unspecified type

GDT_Byte  Eight bit unsigned integer

GDT_UInt16  Sixteen bit unsigned integer

GDT_Int16  Sixteen bit signed integer

GDT_UInt32  Thirty two bit unsigned integer

GDT_Int32  Thirty two bit signed integer

GDT_Float32  Thirty two bit floating point

GDT_Float64  Sixty four bit floating point

GDT_CInt16  Complex Int16

GDT_CInt32  Complex Int32

GDT_CFloat32  Complex Float32

GDT_CFloat64  Complex Float64 **/

KPicInfo * KPicInfo::m_pInstance = NULL;
GDALDataset * KPicInfo::m_pDataset = NULL;
GDALDataset * KPicInfo::m_pHisDataset = NULL;

KPicInfo::KPicInfo()
{
}

void KPicInfo::releaseInstance()
{
    delete m_pInstance;
    m_pInstance = NULL;
}

bool KPicInfo::dataAttach(GDALDataset * dataset)
{
    m_pHisDataset = m_pDataset;

    if(NULL == m_pDataset) m_pDataset = dataset;
    else
    {
        if(!K_CheckDataSetEqu(m_pHisDataset, dataset))
        {
            if(NULL == dataset) return false;
            else m_pDataset = dataset;
        }
        else{
            return false;
        }
    }
    if(NULL != m_pInstance)
    {
        releaseInstance();
    }
    //qDebug()<<"dataset attach";
    return true;
}

void KPicInfo::build()
{
    assert(NULL != m_pDataset);

    m_BandNum = m_pDataset->GetRasterCount();

    GDALRasterBand  *poBand;
    double          adfMinMax[2];
    poBand = m_pDataset->GetRasterBand( 1 );
    int tempMax,tempMin;

//    m_XSize = poBand->GetXSize();
//    m_YSize = poBand->GetYSize();

    m_XSize = m_pDataset->GetRasterXSize();
    m_YSize = m_pDataset->GetRasterYSize();

    qDebug( "size=%dx%d Type=%s BandNum=%d"
            ,m_XSize, m_YSize
            ,GDALGetDataTypeName(m_dataType = poBand->GetRasterDataType())
            ,m_BandNum);
    adfMinMax[0] = poBand->GetMinimum( &tempMin );
    adfMinMax[1] = poBand->GetMaximum( &tempMax );
    if( ! (tempMin && tempMax) )
        GDALComputeRasterMinMax((GDALRasterBandH)poBand, TRUE, adfMinMax);

    qDebug("MinV=%.3fd, MaxV=%.3f",adfMinMax[0], adfMinMax[1]);
    m_vMin = adfMinMax[0];
    m_vMax = adfMinMax[1];

    char ** filelist =m_pDataset->GetFileList();
    // just fetch the first one
    QString temp(*filelist);
    m_fileName = temp;

    m_fileNoExtName = temp.left(temp.lastIndexOf("."));
    m_extName = temp.right(temp.length()-temp.lastIndexOf("."));
    CSLDestroy (filelist);

    m_BandName.clear();

    for(int index = 0;index < m_BandNum;++index)
    {
        poBand = m_pDataset->GetRasterBand( index + 1 );
        QString temp(GDALGetColorInterpretationName(poBand->GetColorInterpretation()));
        m_BandName.push_back(temp);
    }
#ifndef NDEBUG
    for(int index = 0;index < m_BandName.length();++index)
    {
        qDebug()<<m_BandName[index];
    }
#endif
}

KPicInfo * KPicInfo::getInstance()
{
    if(NULL == m_pInstance)
    {
        m_pInstance = new KPicInfo();
    }
    return m_pInstance;
}
