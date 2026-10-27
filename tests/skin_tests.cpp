#include <QtTest>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QPainter>
#include <QUrl>
#include <QtEndian>
#include "core.h"
#include "skinstore.h"
#include "skinmodel.h"

class SkinTests : public QObject {
    Q_OBJECT
  private slots:
    void defaultLibraryContainsMinecraftCharacters() {
        QTemporaryDir root;
        SkinStore store(root.path());
        QCOMPARE(store.skins().size(),9);
        QCOMPARE(store.skins()[0].toMap()["name"].toString(),QString("Steve"));
        QCOMPARE(store.skins()[1].toMap()["name"].toString(),QString("Alex"));
        QCOMPARE(store.skins()[1].toMap()["model"].toString(),QString("slim"));
        QVERIFY(store.error().isEmpty());
        auto image=SkinStore::readSkin(QUrl(store.selectedSkinUrl()).toLocalFile());
        QCOMPARE(image.size(),QSize(64,64));
        QVERIFY(image.textKeys().isEmpty());
    }
    void validatesDimensionsAndPreservesSelection() {
        QTemporaryDir root;
        SkinStore store(root.path());
        QImage invalid(65,64,QImage::Format_ARGB32);invalid.fill(Qt::red);
        const auto path=root.path()+"/bad.png";QVERIFY(invalid.save(path));
        QVERIFY(!store.importSkin(path));
        QCOMPARE(store.selectedSkinId(),QString("default-steve"));
        QVERIFY(!store.error().isEmpty());
        QVERIFY(!store.importSkin("https://example.com/skin.png"));
        QVERIFY(!store.importSkin(path,"unknown"));
    }
    void defaultCharactersCanBeSelectedAndChangeModels() {
        QTemporaryDir root;SkinStore store(root.path());
        for(const auto &value:store.skins()){
            const auto item=value.toMap();store.selectSkin(item["id"].toString());
            QCOMPARE(store.selectedSkinId(),item["id"].toString());
            QCOMPARE(SkinStore::readSkin(QUrl(store.selectedSkinUrl()).toLocalFile()).size(),QSize(64,64));
        }
        store.selectSkin("default-alex");QCOMPARE(store.selectedModel(),QString("slim"));
        store.setSelectedModel("classic");QVERIFY(store.selectedSkinUrl().contains("default-alex-classic.png"));
        SkinStore restored(root.path());QCOMPARE(restored.selectedSkinId(),QString("default-alex"));QCOMPARE(restored.selectedModel(),QString("classic"));
    }
    void importedCopyAndModelSurviveReload() {
        QTemporaryDir root;
        const auto path=root.path()+"/example.png";QVERIFY(SkinStore::defaultTexture().save(path));
        SkinStore store(root.path());QVERIFY(store.importSkin(QUrl::fromLocalFile(path).toString(),"slim"));
        const auto selected=store.selectedSkinId();
        QVERIFY(selected!="default-steve");QCOMPARE(store.selectedModel(),QString("slim"));
        QCOMPARE(moly::fileHash(path),moly::fileHash(QUrl(store.selectedSkinUrl()).toLocalFile()));
        QFile::remove(path);
        SkinStore reloaded(root.path());QCOMPARE(reloaded.selectedSkinId(),selected);QCOMPARE(reloaded.selectedModel(),QString("slim"));
        reloaded.setSelectedModel("classic");QCOMPARE(reloaded.selectedModel(),QString("classic"));
        QSignalSpy apply(&reloaded,&SkinStore::applyRequested);reloaded.requestApply();QCOMPARE(apply.count(),1);
        QCOMPARE(apply.first()[1].toString(),QString("classic"));
    }
    void legacyCannotUseSlimModel() {
        QTemporaryDir root;SkinStore store(root.path());
        QImage legacy(64,32,QImage::Format_ARGB32);legacy.fill(Qt::blue);
        const auto path=root.path()+"/legacy.png";QVERIFY(legacy.save(path));
        QVERIFY(!store.importSkin(path,"slim"));QVERIFY2(store.importSkin(path,"classic"),qPrintable(store.error()));
        store.setSelectedModel("slim");QCOMPARE(store.selectedModel(),QString("classic"));
    }
    void importStripsMetadataAndRejectsOversizedFiles() {
        QTemporaryDir root;SkinStore store(root.path());
        auto image=SkinStore::defaultTexture();image.setText("Comment","private source metadata");
        const auto path=root.path()+"/metadata.png";QVERIFY(image.save(path));
        QVERIFY(!QImage(path).textKeys().isEmpty());
        QVERIFY(SkinStore::readSkin(path).textKeys().isEmpty());
        QVERIFY(store.importSkin(path));
        QVERIFY(QImage(QUrl(store.selectedSkinUrl()).toLocalFile()).textKeys().isEmpty());
        QFile large(root.path()+"/oversized.png");QVERIFY(large.open(QIODevice::WriteOnly));QVERIFY(large.resize(1024*1024+1));large.close();
        QVERIFY(!store.importSkin(large.fileName()));
    }
    void standardFaceMapping() {
        QCOMPARE(SkinModel::textureRect(SkinModel::Head,SkinModel::Front),QRect(8,8,8,8));
        QCOMPARE(SkinModel::textureRect(SkinModel::Body,SkinModel::Front),QRect(20,20,8,12));
        QCOMPARE(SkinModel::textureRect(SkinModel::RightArm,SkinModel::Front,true),QRect(44,20,3,12));
        QCOMPARE(SkinModel::textureRect(SkinModel::LeftArm,SkinModel::Front,false),QRect(36,52,4,12));
        QCOMPARE(SkinModel::textureRect(SkinModel::Head,SkinModel::Front,false,true),QRect(40,8,8,8));
    }
    void paletteAndCompressedMetadataAreHandledSafely() {
        QTemporaryDir root;QImage image(64,64,QImage::Format_Indexed8);
        image.setColorTable({qRgba(0,0,0,0),qRgb(255,0,0)});image.fill(1);image.setPixel(0,0,0);
        const auto path=root.path()+"/palette.png";QVERIFY(image.save(path));
        QFile file(path);QVERIFY(file.open(QIODevice::ReadOnly));auto original=file.readAll();file.close();
        QByteArray payload("Comment\0",8);payload.append(char(0));payload+=qCompress(QByteArray(4*1024*1024,'x'),9).mid(4);
        QByteArray chunk(4,'\0');qToBigEndian<quint32>(quint32(payload.size()),reinterpret_cast<uchar*>(chunk.data()));
        const auto data=QByteArray("zTXt")+payload;chunk+=data;
        quint32 crc=0xffffffffu;for(uchar value:data){crc^=value;for(int bit=0;bit<8;++bit)crc=(crc>>1)^((crc&1)?0xedb88320u:0u);}crc^=0xffffffffu;
        QByteArray check(4,'\0');qToBigEndian<quint32>(crc,reinterpret_cast<uchar*>(check.data()));chunk+=check;
        original.insert(33,chunk);moly::writeBytes(path,original);
        const auto clean=SkinStore::readSkin(path);QVERIFY(clean.textKeys().isEmpty());
        QCOMPARE(clean.pixelColor(0,0).alpha(),0);QCOMPARE(clean.pixelColor(1,1),QColor(Qt::red));
        QByteArray malformed=original;malformed.replace(33,4,QByteArray::fromHex("7fffffff"));moly::writeBytes(path,malformed);
        QVERIFY_EXCEPTION_THROWN(SkinStore::readSkin(path),moly::Error);
    }
    void rendererProjectsRealTexture() {
        SkinModel model;model.setWidth(230);model.setHeight(360);model.setYaw(0);
        QImage image(230,360,QImage::Format_ARGB32_Premultiplied);image.fill(Qt::transparent);
        QPainter painter(&image);model.paint(&painter);painter.end();
        QVERIFY(qAlpha(image.pixel(115,40))>0);
        // Head and shirt produce distinct textured regions, not a flat placeholder.
        QVERIFY(image.pixelColor(115,70)!=image.pixelColor(115,170));
    }
    void rotationShowsTheOppositeSkinFace() {
        QTemporaryDir root;auto texture=SkinStore::defaultTexture();
        {QPainter painter(&texture);painter.setCompositionMode(QPainter::CompositionMode_Source);painter.fillRect(QRect(32,0,32,16),Qt::transparent);painter.fillRect(QRect(8,8,8,8),Qt::red);painter.fillRect(QRect(24,8,8,8),Qt::blue);}
        const auto path=root.path()+"/faces.png";QVERIFY(texture.save(path));
        SkinModel model;model.setWidth(230);model.setHeight(360);model.setSource(QUrl::fromLocalFile(path));
        auto render=[&]{QImage result(230,360,QImage::Format_ARGB32_Premultiplied);result.fill(Qt::transparent);QPainter painter(&result);model.paint(&painter);return result;};
        model.setYaw(0);auto front=render();QVERIFY(front.pixelColor(115,60).red()>180);QVERIFY(front.pixelColor(115,60).blue()<20);
        model.setYaw(180);auto back=render();QVERIFY(back.pixelColor(115,60).blue()>180);QVERIFY(back.pixelColor(115,60).red()<20);
    }
};
QTEST_MAIN(SkinTests)
#include "skin_tests.moc"
