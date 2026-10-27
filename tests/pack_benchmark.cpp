#include <QCoreApplication>
#include <QElapsedTimer>
#include <QTextStream>
#include "content.h"
int main(int argc,char **argv){
    QCoreApplication app(argc,argv);QTextStream out(stdout);
    if(argc!=3){out<<"Usage: pack_benchmark <archive> <isolated-cache-root>\n";return 2;}
    try{
        QElapsedTimer timer;timer.start();
        moly::Context context;context.progress=[&](QString text,qint64 done,qint64 total){if(total>0&&done==total)out<<text<<Qt::endl;};
        const auto plan=moly::Content(QString::fromLocal8Bit(argv[2]),{},context).pack(QString::fromLocal8Bit(argv[1]));
        out<<"Prepared "<<plan["files"].toArray().size()<<" files in "<<timer.elapsed()<<" ms"<<Qt::endl;return 0;
    }catch(const std::exception &error){out<<error.what()<<Qt::endl;return 1;}
}
