#ifndef KPICINFO_H
#define KPICINFO_H

#include "gdal_priv.h"
#include <QString>
#include <QStringList>

class KPicInfo
{
private:
    KPicInfo();
    KPicInfo(const KPicInfo &);
    KPicInfo & operator=(const KPicInfo &);
    static KPicInfo * m_pInstance;
    static GDALDataset * m_pDataset;
    static GDALDataset * m_pHisDataset;
    static void releaseInstance();
public:
    static bool beEcho;
    static KPicInfo * getInstance();
    static bool dataAttach(GDALDataset *,bool=false);
    void build();
    inline double getMin(){ return m_vMin; }
    inline double getMax(){ return m_vMax; }
    inline int getBandNum(){ return m_BandNum; }
    inline int getWidth(){ return m_XSize; }
    inline int getHeight(){ return m_YSize; }
    inline GDALDataType getType(){ return m_dataType; }
    inline QString getFileExtName() { return m_extName; }
    inline QString getFileNoExtName() { return m_fileNoExtName; }
    inline QString getFileName() { return m_fileName; }
    inline QStringList getBandName() { return m_BandName; }
private:
    int m_BandNum;
    int m_XSize;
    int m_YSize;
    double m_vMax;
    double m_vMin;
    QString m_extName;
    QString m_fileNoExtName;
    QString m_fileName;
    QStringList m_BandName;
    GDALDataType m_dataType;
};

#endif // KPICINFO_H
