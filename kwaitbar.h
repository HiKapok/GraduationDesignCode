#ifndef KWAITBAR_H
#define KWAITBAR_H

#include <QString>
#include <QThread>
#include <iostream>
#include <fstream>

#define K_WAITBAR_START(waitbar) if(true != KWaitBar::sBeRunning){ waitbar.start(); }
#define K_WAITBAR_END(waitbar) do { waitbar.finish(); waitbar.locked = false; while(waitbar.isRunning()); } while(0)

typedef enum {
    WaitBar_Run = 0,
    WaitBar_Cancel = 1,
    WaitBar_Finish = 2,
} K_WaitBarTypes;

// the nested waitbar is hiden automatic
class KWaitBar : public QThread
{
    void run() Q_DECL_OVERRIDE{
        autoRun();
    }
public:
    KWaitBar(QString tip, int totalItems,unsigned int step=2000)
        :locked(true),
        m_tips(tip),
        m_iTotalItems(totalItems>8 ? 8 : totalItems),
        m_iNowPos(0),
        m_iHisNowPos(-1),
        m_tRunning(WaitBar_Run),
        m_step(step){}
    inline void cancel(){ m_iNowPos = m_iTotalItems; m_tRunning = WaitBar_Cancel; }
    inline void update(){
        static unsigned int temp=0;
        temp++;
        if(temp>m_step){ m_iNowPos++; m_iNowPos= (m_iNowPos % m_iTotalItems); temp=0; }
    }
    inline void finish(){ m_iNowPos = m_iTotalItems; m_tRunning = WaitBar_Finish; }
    void autoRun(){
        if (true != sBeRunning){
            sBeRunning = true;
            std::cout << "\r";
            while (1)
            {
                if (m_iHisNowPos != m_iNowPos)
                {
                    m_iHisNowPos = m_iNowPos;

                    std::cout << m_tips.toStdString();
                    int index = 0;
                    for (; index < m_iNowPos; ++index)
                    {
                        std::cout << ".";
                    }
                    for (; index < m_iTotalItems; ++index)
                    {
                        std::cout << " ";
                    }
                    if (m_tRunning == WaitBar_Finish){ std::cout << "[Finished]" << std::endl; break; }
                    else if (m_tRunning == WaitBar_Cancel){ std::cout << "[Canceled]" << std::endl; break; }
                    std::cout << "\r";
                    std::cout.flush();
                    //this->sleep(1);
                }
            }
            while (locked);
            sBeRunning = false;
        }
    }
    bool locked;
    static bool sBeRunning;
private:
    QString m_tips;
    int m_iTotalItems;
    int m_iNowPos;
    int m_iHisNowPos;
    K_WaitBarTypes m_tRunning;
    unsigned int m_step;
    void *operator new(std::size_t){ return NULL; }
        void operator delete(void *){}
    void *operator new[](std::size_t){ return NULL; }
        void operator delete[](void *){}
};
bool KWaitBar::sBeRunning = false;

#endif // KWAITBAR_H

