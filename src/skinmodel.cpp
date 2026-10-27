#include "skinmodel.h"
#include "skinstore.h"
#include <QPainter>
#include <QPainterPath>
#include <QVector3D>
#include <QtMath>
#include <algorithm>
#include <array>

SkinModel::SkinModel(QQuickItem *parent) : QQuickPaintedItem(parent), texture(SkinStore::defaultTexture()) {
    setAntialiasing(true); setOpaquePainting(false);
    connect(this, &QQuickItem::widthChanged, this, [this] { update(); });
    connect(this, &QQuickItem::heightChanged, this, [this] { update(); });
}
void SkinModel::setSource(const QUrl &url) {
    if (url == skinSource) return;
    skinSource = url;
    try { texture = url.isLocalFile() ? SkinStore::readSkin(url.toLocalFile()) : SkinStore::defaultTexture(); }
    catch (...) { texture = SkinStore::defaultTexture(); }
    emit sourceChanged(); update();
}
void SkinModel::setYaw(qreal value) { if (qFuzzyCompare(rotation + 1, value + 1)) return; rotation=value; emit yawChanged(); update(); }
void SkinModel::setPose(qreal value) { if (qFuzzyCompare(motionPose + 1, value + 1)) return; motionPose=value; emit poseChanged(); update(); }
void SkinModel::setSlim(bool value) { if (slimModel == value) return; slimModel=value; emit slimChanged(); update(); }

QRect SkinModel::textureRect(Part part, Face face, bool slim, bool overlay) {
    const int w=(part==Head||part==Body)?8:(part==RightArm||part==LeftArm)&&slim?3:4;
    const int h=part==Head?8:12, d=part==Head?8:4;
    QPoint uv;
    switch(part) {
    case Head: uv=overlay?QPoint(32,0):QPoint(0,0);break;
    case Body: uv=overlay?QPoint(16,32):QPoint(16,16);break;
    case RightArm:uv=overlay?QPoint(40,32):QPoint(40,16);break;
    case LeftArm:uv=overlay?QPoint(48,48):QPoint(32,48);break;
    case RightLeg:uv=overlay?QPoint(0,32):QPoint(0,16);break;
    case LeftLeg:uv=overlay?QPoint(0,48):QPoint(16,48);break;
    }
    switch(face) {
    case Front:return QRect(uv+QPoint(d,d),QSize(w,h));
    case Right:return QRect(uv+QPoint(0,d),QSize(d,h));
    case Back:return QRect(uv+QPoint(2*d+w,d),QSize(w,h));
    case Left:return QRect(uv+QPoint(d+w,d),QSize(d,h));
    case Top:return QRect(uv+QPoint(d,0),QSize(w,d));
    case Bottom:return QRect(uv+QPoint(d+w,0),QSize(w,d));
    }
    return {};
}
void SkinModel::paint(QPainter *painter) {
    if (width() < 1 || height() < 1) return;
    struct DrawFace { QPolygonF polygon; QRect texture; qreal depth, light; bool mirror; };
    QVector<DrawFace> faces;
    const float yawRadians=qDegreesToRadians(float(rotation));
    const float pitch=qDegreesToRadians(10.0f), cy=qCos(yawRadians), sy=qSin(yawRadians);
    const float cp=qCos(pitch), sp=qSin(pitch);
    const qreal unit=qMin(width()/23.0,height()/36.0);
    auto rotate=[&](QVector3D v){return QVector3D(v.x()*cy+v.z()*sy,v.y(),-v.x()*sy+v.z()*cy);};
    auto project=[&](QVector3D v){v=rotate(v);return QPointF(width()/2+v.x()*unit,height()/2-(v.y()-16)*cp*unit+v.z()*sp*unit);};
    const std::array<QVector3D,6> normals={QVector3D(0,0,1),QVector3D(-1,0,0),QVector3D(0,0,-1),QVector3D(1,0,0),QVector3D(0,1,0),QVector3D(0,-1,0)};
    auto box=[&](Part part,float x0,float x1,float y0,float y1,float z0,float z1,bool overlay){
        const float swing=(part==RightArm||part==LeftLeg?1:-1)*float(qSin(motionPose))*3.0f;
        const bool limb=part==RightArm||part==LeftArm||part==RightLeg||part==LeftLeg;
        const float angle=limb?qDegreesToRadians(swing):0;
        const float pivot=y1;
        const std::array<std::array<QVector3D,4>,6> vertices={{
            {QVector3D(x0,y1,z1),QVector3D(x1,y1,z1),QVector3D(x1,y0,z1),QVector3D(x0,y0,z1)},
            {QVector3D(x0,y1,z0),QVector3D(x0,y1,z1),QVector3D(x0,y0,z1),QVector3D(x0,y0,z0)},
            {QVector3D(x1,y1,z0),QVector3D(x0,y1,z0),QVector3D(x0,y0,z0),QVector3D(x1,y0,z0)},
            {QVector3D(x1,y1,z1),QVector3D(x1,y1,z0),QVector3D(x1,y0,z0),QVector3D(x1,y0,z1)},
            {QVector3D(x0,y1,z0),QVector3D(x1,y1,z0),QVector3D(x1,y1,z1),QVector3D(x0,y1,z1)},
            {QVector3D(x0,y0,z1),QVector3D(x1,y0,z1),QVector3D(x1,y0,z0),QVector3D(x0,y0,z0)}
        }};
        for(int face=0;face<6;++face){
            auto normal=rotate(normals[face]);
            const qreal facing=normal.z()*cp+normal.y()*sp;
            if(facing<=0.005)continue;
            bool mirror=false;
            auto uvPart=part;
            auto uvFace=Face(face);
            if(texture.height()==32&&(part==LeftArm||part==LeftLeg)){
                uvPart=part==LeftArm?RightArm:RightLeg;mirror=true;
                if(uvFace==Right)uvFace=Left;else if(uvFace==Left)uvFace=Right;
            }
            const auto uv=textureRect(uvPart,uvFace,slimModel,overlay);
            if(!texture.rect().contains(uv))continue;
            QPolygonF polygon;qreal depth=0;
            for(auto point:vertices[face]){
                if(limb){const float y=point.y()-pivot,z=point.z();point.setY(pivot+y*qCos(angle)-z*qSin(angle));point.setZ(y*qSin(angle)+z*qCos(angle));}
                const auto r=rotate(point);depth+=r.z()*cp+r.y()*sp;polygon<<project(point);
            }
            faces.append({polygon,uv,depth/4,face==Top?1.0:face==Front?0.98:0.79,mirror});
        }
    };
    auto modelLayer=[&](bool overlay){
        const float grow=overlay?0.28f:0.0f;
        auto part=[&](Part p,float x0,float x1,float y0,float y1,float z0,float z1){box(p,x0-grow,x1+grow,y0-grow,y1+grow,z0-grow,z1+grow,overlay);};
        part(Head,-4,4,24,32,-4,4);part(Body,-4,4,12,24,-2,2);
        const float arm=slimModel?3:4;
        part(RightArm,-4-arm,-4,12,24,-2,2);part(LeftArm,4,4+arm,12,24,-2,2);
        part(RightLeg,-4,0,0,12,-2,2);part(LeftLeg,0,4,0,12,-2,2);
    };
    modelLayer(false);
    if(texture.height()==64)modelLayer(true);else box(Head,-4.28,4.28,23.72,32.28,-4.28,4.28,true);
    std::stable_sort(faces.begin(),faces.end(),[](const DrawFace &a,const DrawFace &b){return a.depth<b.depth;});
    painter->setRenderHint(QPainter::SmoothPixmapTransform,false);
    painter->setPen(Qt::NoPen);
    for(const auto &face:faces){
        auto tile=texture.copy(face.texture);
        if(face.mirror)tile=tile.flipped(Qt::Horizontal);
        // Lighting is applied to non-transparent texels before projection so
        // transparent clothing-layer texels never darken the base model.
        if(face.light<1)for(int y=0;y<tile.height();++y){auto line=reinterpret_cast<QRgb*>(tile.scanLine(y));for(int x=0;x<tile.width();++x){auto c=QColor::fromRgba(line[x]);c.setRed(qRound(c.red()*face.light));c.setGreen(qRound(c.green()*face.light));c.setBlue(qRound(c.blue()*face.light));line[x]=c.rgba();}}
        QTransform transform;
        const QPolygonF source={QPointF(0,0),QPointF(tile.width(),0),QPointF(tile.width(),tile.height()),QPointF(0,tile.height())};
        if(!QTransform::quadToQuad(source,face.polygon,transform))continue;
        painter->save();painter->setTransform(transform,true);painter->drawImage(QPointF(0,0),tile);painter->restore();
    }
}
