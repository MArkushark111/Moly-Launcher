#include "artwork.h"
#include "core.h"
#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QUrl>
#include <QPainter>
#include <QPainterPath>

ArtworkView::ArtworkView(QQuickItem *parent):QQuickPaintedItem(parent),movie_(this){
    setAntialiasing(true);movie_.setCacheMode(QMovie::CacheNone);
    connect(&movie_,&QMovie::frameChanged,this,[this]{update();});
    connect(this,&QQuickItem::visibleChanged,this,[this]{if(animated_&&!movie_.fileName().isEmpty()&&movie_.isValid())movie_.setPaused(!isVisible());});
}
void ArtworkView::setSource(const QUrl &url){if(source_==url)return;source_=url;reload();emit appearanceChanged();}
void ArtworkView::setRadius(qreal value){radius_=qMax(0.0,value);update();emit appearanceChanged();}
void ArtworkView::setShade(qreal value){shade_=qBound(0.0,value,1.0);update();emit appearanceChanged();}
void ArtworkView::setAnimated(bool value){if(animated_==value)return;animated_=value;reload();emit appearanceChanged();}
void ArtworkView::reload(){
    movie_.stop();movie_.setFileName({});still_={};
    if(!source_.isLocalFile()){update();return;}
    const auto path=source_.toLocalFile();QImageReader reader(path);const auto size=reader.size();
    if(animated_&&reader.format().toLower()=="gif"){
        movie_.setFileName(path);movie_.setScaledSize(size.scaled(512,512,Qt::KeepAspectRatio));movie_.start();movie_.setPaused(!isVisible());
    }else{reader.setScaledSize(size.scaled(512,512,Qt::KeepAspectRatio));still_=reader.read();}
    update();
}
void ArtworkView::paint(QPainter *painter){
    const auto image=!movie_.fileName().isEmpty()?movie_.currentImage():still_;
    if(image.isNull()||width()<=0||height()<=0)return;
    QPainterPath clip;clip.addRoundedRect(boundingRect(),radius_,radius_);painter->setClipPath(clip);painter->setRenderHint(QPainter::SmoothPixmapTransform);
    const auto size=QSizeF(image.size()).scaled(boundingRect().size(),Qt::KeepAspectRatioByExpanding);
    painter->drawImage(QRectF((width()-size.width())/2,(height()-size.height())/2,size.width(),size.height()),image);
    painter->fillPath(clip,QColor(0,0,0,qRound(shade_*255)));
}

namespace moly {
QString prepareArtwork(const QString &value, const QString &directory, bool background) {
    const QUrl url(value);
    const auto path=url.isLocalFile()?url.toLocalFile():value;
    const QFileInfo info(path);
    if ((!url.isLocalFile()&&!info.isAbsolute()) || !info.isFile() || info.isSymLink())
        throw Error("Choose a local image or GIF.");
    if(info.size()>16*1024*1024)throw Error("Choose artwork smaller than 16 MB.");
    QImageReader reader(path);reader.setAutoTransform(true);
    if(!QList<QByteArray>{"png","jpeg","gif","webp","bmp"}.contains(reader.format().toLower()))
        throw Error("Choose a PNG, JPEG, GIF, WebP, or BMP image.");
    const auto size=reader.size();
    if(!size.isValid()||size.width()>8192||size.height()>8192||qint64(size.width())*size.height()>16*1024*1024)
        throw Error("Choose artwork up to 8192 pixels per side and 16 megapixels.");
    const bool animated=reader.format().toLower()=="gif" && reader.supportsAnimation();
    const auto stem=background?QString("background"):QString("icon");
    if(animated) {
        const int count=reader.imageCount();
        if(count<1||count>300||qint64(size.width())*size.height()*count>32ll*1024*1024)
            throw Error("This GIF is too large to animate. Use up to 300 frames and 32 million total frame pixels.");
        qint64 decodedPixels=0;
        for(int frame=0;frame<count;++frame){
            const auto image=reader.read();
            if(image.isNull())throw Error("This GIF has an unreadable frame. Choose another animation.");
            decodedPixels+=qint64(image.width())*image.height();
            if(decodedPixels>32ll*1024*1024)throw Error("This GIF is too large to animate safely.");
        }
        QFile file(path);if(!file.open(QIODevice::ReadOnly))throw Error("Cannot read the selected GIF.");
        writeBytes(safePath(directory,stem+".gif"),file.readAll());
        return stem+".gif";
    }
    const QSize bound=background?QSize(1920,1080):QSize(512,512);
    reader.setScaledSize(size.scaled(bound,Qt::KeepAspectRatio));
    auto picture=reader.read();if(picture.isNull())throw Error("This image could not be opened. Try PNG, JPEG, WebP, or GIF.");
    picture=picture.scaled(bound,Qt::KeepAspectRatio,Qt::SmoothTransformation);
    for(const auto &key:picture.textKeys())picture.setText(key,{});
    QByteArray bytes;QBuffer buffer(&bytes);buffer.open(QIODevice::WriteOnly);
    if(!picture.save(&buffer,"PNG"))throw Error("The artwork could not be prepared.");
    writeBytes(safePath(directory,stem+".png"),bytes);
    return stem+".png";
}
QString artworkUrl(const QString &directory,const QString &filename,const QString &revision) {
    if(!QStringList{"icon.png","icon.gif","background.png","background.gif"}.contains(filename))return {};
    const auto path=safePath(directory,filename);
    if(!QFileInfo(path).isFile()||QFileInfo(path).isSymLink())return {};
    auto url=QUrl::fromLocalFile(path);url.setQuery("v="+revision);return url.toString();
}
}
