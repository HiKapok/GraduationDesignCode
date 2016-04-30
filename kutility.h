#ifndef KUTILITY_H
#define KUTILITY_H

#include <QString>
#include <map>

class KUtility
{
private:
    // no instance of this class
    KUtility();
    KUtility(const KUtility &);
    KUtility & operator=(const KUtility &);
    void *operator new(size_t);
    void *operator new[](size_t);
    void operator delete(void*);
    void operator delete[](void*);
public:
    static QString getDirRoot(QString);
    static bool checkDirName(QString &);
    static void buildInputList(std::map<QString,int>&,QString);
};

#endif // KUTILITY_H
