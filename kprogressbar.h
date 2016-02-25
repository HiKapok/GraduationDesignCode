#ifndef KPROGRESSBAR_H
#define KPROGRESSBAR_H

#include <QString>
#include <QThread>

/**  how to use a progress bar in the terminal
  *
  *  KProgressBar temp("calculating",30);
  *  K_PROGRESS_START(temp);
  *  for(int x = 0;x<30;++x)
  *  {
  *      temp.autoUpdate();
  *      if(x==19)temp.cancel();
  *      //TODO: do something here
  *  }
  *  K_PROGRESS_END(temp);
  */

#define K_PROGRESS_START(progress) progress.start()
#define K_PROGRESS_END(progress) do { progress.finish(); while(progress.isRunning()); } while(0)

class KProgressBar : public QThread
{
    void run() Q_DECL_OVERRIDE {
        autoRun();
    }
public:
    typedef enum {
        Progress_Run = 0,
        Progress_Cancel = 1,
        Progress_Finish = 2,
    } K_ProRunTypes;

    KProgressBar(QString,unsigned long int, int=60);
    inline void cancel(){ m_tRunning=Progress_Cancel; }
    inline int getNowPos(){ return m_iNowPos; }
    inline void updateNowPos(int pos){ if(pos > m_iNowPos && pos <= m_iTotalItems) { m_iNowPos = pos; m_fNowPos=m_iNowPos; } }
    void autoUpdate(){ m_fNowPos = m_fNowPos + m_fUpdateFreq; m_iNowPos = m_fNowPos+.5; if(m_iNowPos > m_iTotalItems) m_iNowPos = m_iTotalItems; }
    void autoRun();
    inline void finish(){ if(Progress_Run==m_tRunning) m_iNowPos=m_iTotalItems; }
private:
    static int m_siHisWidth;
    QString m_tips;
    unsigned long int  m_iTotalSteps;
    int m_iTotalItems;
    int m_iNowPos;
    int m_iHisNowPos;
    float m_fNowPos;
    float m_fUpdateFreq;
    K_ProRunTypes m_tRunning;
    void *operator new(std::size_t){ return NULL; }
    void operator delete(void *){}
    void *operator new[](std::size_t){ return NULL; }
    void operator delete[](void *){}
};

#endif // KPROGRESSBAR_H
