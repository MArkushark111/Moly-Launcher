#include "artwork.h"
#include <QtTest>
#include <QBuffer>
#include <QImage>
#include <QPainter>
#include <QFile>
#include <QDir>
#include <QStringConverter>

class ArtworkTests:public QObject {
    Q_OBJECT
    static QString imageUrl(QSize size) {
        QImage image(size,QImage::Format_ARGB32);image.fill(Qt::green);QByteArray bytes;QBuffer buffer(&bytes);buffer.open(QIODevice::WriteOnly);image.save(&buffer,"PNG");
        return "data:image/png;base64,"+QString::fromLatin1(bytes.toBase64());
    }
private slots:
    void interfaceSourcesAreValidUtf8() {
        const QDir sources(QStringLiteral(MOLY_QML_DIR));QVERIFY(sources.exists());
        for(const auto &name:sources.entryList({"*.qml"},QDir::Files)) {
            QFile file(sources.filePath(name));QVERIFY(file.open(QIODevice::ReadOnly));QStringDecoder decoder(QStringDecoder::Utf8);const QString decoded=decoder(file.readAll());
            QVERIFY2(!decoder.hasError(),qPrintable("Invalid UTF-8 in "+name));QVERIFY2(!decoded.contains(QChar(0xfffd)),qPrintable("Replacement glyph in "+name));
        }
    }
    void serverImageRendersWithRoundedCorners() {
        ArtworkView item;item.setWidth(64);item.setHeight(64);item.setRadius(12);item.setShade(0);item.setSource(QUrl(imageUrl({64,64})));QVERIFY(item.ready());
        QImage rendered(64,64,QImage::Format_ARGB32);rendered.fill(Qt::transparent);QPainter painter(&rendered);item.paint(&painter);painter.end();
        QCOMPARE(rendered.pixelColor(32,32),QColor(Qt::green));QCOMPARE(rendered.pixelColor(0,0).alpha(),0);
    }
    void invalidAndOversizedImagesHaveAFallback() {
        ArtworkView item;item.setSource(QUrl("data:image/png;base64,broken"));QVERIFY(!item.ready());
        item.setSource(QUrl(imageUrl({257,64})));QVERIFY(!item.ready());item.setSource(QUrl(imageUrl({64,64})));QVERIFY(item.ready());item.setSource(QUrl());QVERIFY(!item.ready());
    }
};
QTEST_GUILESS_MAIN(ArtworkTests)
#include "artwork_tests.moc"
