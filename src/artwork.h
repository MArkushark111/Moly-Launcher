#pragma once
#include <QString>
#include <QQuickPaintedItem>
#include <QMovie>
#include <QUrl>

class ArtworkView : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY appearanceChanged)
    Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY appearanceChanged)
    Q_PROPERTY(qreal shade READ shade WRITE setShade NOTIFY appearanceChanged)
    Q_PROPERTY(bool animated READ animated WRITE setAnimated NOTIFY appearanceChanged)
    Q_PROPERTY(bool ready READ ready NOTIFY appearanceChanged)
public:
    explicit ArtworkView(QQuickItem *parent=nullptr);
    QUrl source()const{return source_;} qreal radius()const{return radius_;} qreal shade()const{return shade_;} bool animated()const{return animated_;}
    bool ready()const{return !still_.isNull()||(!movie_.fileName().isEmpty()&&movie_.isValid());}
    void setSource(const QUrl &source);void setRadius(qreal value);void setShade(qreal value);void setAnimated(bool value);
    void paint(QPainter *painter) override;
signals:
    void appearanceChanged();
private:
    QUrl source_;QMovie movie_;QImage still_;qreal radius_=17,shade_=0.5;bool animated_=true;
    void reload();
};

namespace moly {
// Copies validated local artwork into an instance; static images are normalized.
QString prepareArtwork(const QString &source, const QString &instanceDirectory, bool background);
QString artworkUrl(const QString &directory, const QString &filename, const QString &revision);
}
