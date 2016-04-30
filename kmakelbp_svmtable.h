#ifndef KMAKELBPSVMTABLE_H
#define KMAKELBPSVMTABLE_H

#include <QString>
#include <QDebug>
#include <vector>
#include <map>
#include <utility>
#include <algorithm>

using std::vector;
using std::map;
using std::pair;

class KMakeLBP_SVMTable
{
public:
    KMakeLBP_SVMTable(map<QString,int>,map<QString,int>,QString,bool = false);
    KMakeLBP_SVMTable(QString,QString,QString,bool = false);
    void makeTable();
    QString getRootDir(){ return m_sOutRoot; }
    QString getTrainFile(){ return m_trainFileName; }
    QString getTestFile(){ return m_testFileName; }
    static bool useImprovedLBP;
private:
    map<QString,int> m_vecInput;
    map<QString,int> m_vecTestInput;
    map<QString,int> m_vecPyramid;
    map<QString,int> m_vecTestPyramid;
    //QString m_sOutput;
    QString m_sOutRoot;
    QString m_trainFileName;
    QString m_testFileName;
    void makeTrainTable();
    void makeTestTable();
    void buildAllPyramid(map<QString,int> &,map<QString,int>&,bool &,bool=true);
};

#endif // KMAKELBPSVMTABLE_H
