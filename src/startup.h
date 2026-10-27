#pragma once
#include <QObject>
#include <QColor>
#include <QElapsedTimer>

// Progress counts completed local initialization stages; it never waits for a
// catalog request and has no minimum display time.
class StartupState:public QObject {
    Q_OBJECT
    Q_PROPERTY(int completed READ completed NOTIFY changed)
    Q_PROPERTY(int total READ total CONSTANT)
    Q_PROPERTY(qreal progress READ progress NOTIFY changed)
    Q_PROPERTY(QString phase READ phase NOTIFY changed)
    Q_PROPERTY(bool ready READ ready NOTIFY changed)
    Q_PROPERTY(bool interfaceVisible READ interfaceVisible NOTIFY changed)
    Q_PROPERTY(QColor accent READ accent NOTIFY changed)
    Q_PROPERTY(bool reducedMotion READ reducedMotion NOTIFY changed)
public:
    explicit StartupState(QObject *parent=nullptr):QObject(parent){elapsed_.start();}
    int completed()const{return completed_;} int total()const{return 4;}
    qreal progress()const{return qreal(completed_)/total();}
    QString phase()const{return phase_;} bool ready()const{return completed_==total();}
    bool interfaceVisible()const{return visible_;}
    QColor accent()const{return accent_;} bool reducedMotion()const{return reduced_;}
    qint64 elapsedMs()const{return elapsed_.elapsed();}
    void configure(QColor accent,bool reduced){if(accent.isValid())accent_=accent;reduced_=reduced;emit changed();}
    void next(const QString &phase){if(completed_<total())++completed_;phase_=phase;emit changed();}
    void showInterface(){visible_=true;emit changed();}
signals:
    void changed();
private:
    int completed_=0;bool visible_=false,reduced_=false;
    QString phase_="Reading your library";QColor accent_{QStringLiteral("#8b5cf6")};QElapsedTimer elapsed_;
};
