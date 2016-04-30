#ifndef KSVMCONTROLLER_H
#define KSVMCONTROLLER_H

#include <QString>
#include <map>

typedef enum {
    Feature_LBP = 0,
    Feature_ImprovedLBP = 1,
    Feature_Tamura = 2,
    Feature_Gabor = 3
} K_FeatureType;

class KSVMController
{
public:
    KSVMController(QString,QString,QString,K_FeatureType=Feature_LBP);
    void build();
    void classProc();
private:
    std::map<QString,int> m_vecTrainInput;
    std::map<QString,int> m_vecTestInput;
    QString m_sOutFile;
    K_FeatureType m_featureType;
    QString m_sOutRoot;

    void createTable(std::map<QString,int>&,QString);
};

#endif // KSVMCONTROLLER_H
