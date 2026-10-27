#pragma once
#include <QQuickPaintedItem>
#include <QImage>
#include <QUrl>

class SkinModel : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(qreal yaw READ yaw WRITE setYaw NOTIFY yawChanged)
    Q_PROPERTY(qreal pose READ pose WRITE setPose NOTIFY poseChanged)
    Q_PROPERTY(bool slim READ slim WRITE setSlim NOTIFY slimChanged)
  public:
    enum Part { Head, Body, RightArm, LeftArm, RightLeg, LeftLeg };
    enum Face { Front, Right, Back, Left, Top, Bottom };
    explicit SkinModel(QQuickItem *parent = nullptr);
    void paint(QPainter *painter) override;
    QUrl source() const { return skinSource; }
    qreal yaw() const { return rotation; }
    qreal pose() const { return motionPose; }
    bool slim() const { return slimModel; }
    void setSource(const QUrl &url);
    void setYaw(qreal degrees);
    void setPose(qreal value);
    void setSlim(bool slim);
    static QRect textureRect(Part part, Face face, bool slim = false, bool overlay = false);
  signals:
    void sourceChanged(); void yawChanged(); void poseChanged(); void slimChanged();
  private:
    QUrl skinSource;
    QImage texture;
    qreal rotation = -24, motionPose = 0;
    bool slimModel = false;
};
