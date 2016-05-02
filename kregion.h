#ifndef KREGION_H
#define KREGION_H

#include <list>

using std::list;

class KSLE
{
public:
    KSLE(int line,int start,int end):m_line(line),m_startCol(start),m_endCol(end){}
    KSLE(const KSLE &rhs){
        this->m_line = rhs.m_line;
        this->m_startCol = rhs.m_startCol;
        this->m_endCol = rhs.m_endCol;
    }
    KSLE & operator=(const KSLE& rhs){
        this->m_line = rhs.m_line;
        this->m_startCol = rhs.m_startCol;
        this->m_endCol = rhs.m_endCol;
        return *this;
    }
    bool operator==(const KSLE &rhs) const{
        return (this->m_line==rhs.m_line)&&(this->m_endCol==rhs.m_endCol)&&(this->m_startCol==rhs.m_startCol);
    }
    bool operator<(const KSLE &rhs) const{
        return (this->m_line<rhs.m_line)||(this->m_line==rhs.m_line&&this->m_startCol<rhs.m_startCol);
    }
    bool operator!=(const KSLE &rhs) const{
        return (this->m_line!=rhs.m_line)||(this->m_endCol!=rhs.m_endCol)||(this->m_startCol!=rhs.m_startCol);
    }
    bool operator>(const KSLE &rhs) const{
        return (this->m_line>rhs.m_line)||(this->m_line==rhs.m_line&&this->m_startCol>rhs.m_startCol);
    }
    KSLE& operator+=(const KSLE& rhs){
        m_endCol = rhs.m_endCol;
        return *this;
    }

    inline int getLine() const{ return m_line; }
    inline int getStartCol() const{ return m_startCol; }
    inline int getEndCol() const{ return m_endCol; }
    inline void setLine(int line){ m_line=line; }
    inline void setStartCol(int col){ m_startCol=col; }
    inline void setEndCol(int col){ m_endCol=col; }
private:
    int m_line;
    int m_startCol;
    int m_endCol;
};

class KRegion
{
public:
    KRegion(long id):m_id(id){ m_regLists.clear(); }
    KRegion& operator+=(const KRegion&);
    //const KRegion operator+(const KRegion&, const KRegion&);
    long getRegID(){ return m_id; }
    bool operator==(const KRegion &rhs) const{
        return this->m_id==rhs.m_id;
    }
    bool operator<(const KRegion &rhs) const{
        return this->m_id<rhs.m_id;
    }
    KRegion(const KRegion &rhs){
        this->m_id = rhs.m_id;
        this->m_regLists = rhs.m_regLists;
    }
    KRegion & operator=(const KRegion& rhs){
        this->m_id = rhs.m_id;
        this->m_regLists = rhs.m_regLists;
        return *this;
    }
    int getMinLine();
    int getMaxLine();
    bool isSinglePixel();
    int totalPixels();
    long getID(){ return m_id; }
    bool hasPoint(int,int);
    void pushLine(KSLE& sle){ m_regLists.push_back(sle); }
    void rmLine(KSLE& sle){ m_regLists.remove(sle); }
    const list<KSLE> &getLists() const{ return m_regLists; }
private:
    KRegion();
    long m_id;
    list<KSLE> m_regLists;
};

#endif // KREGION_H
