#include "startup.h"
#include <QtTest>
class StartupTests:public QObject {
    Q_OBJECT
private slots:
    void progressFollowsActualCompletedStages() {
        StartupState state;QCOMPARE(state.completed(),0);QVERIFY(!state.ready());QCOMPARE(state.progress(),0.0);
        state.configure(QColor("#3b82f6"),true);QCOMPARE(state.accent(),QColor("#3b82f6"));QVERIFY(state.reducedMotion());
        for(int step=1;step<=3;++step){state.next("Local stage");QCOMPARE(state.completed(),step);QVERIFY(!state.ready());QCOMPARE(state.progress(),qreal(step)/4);}
        state.showInterface();QVERIFY(state.interfaceVisible());QVERIFY(!state.ready());state.next("First frame");QVERIFY(state.ready());QCOMPARE(state.progress(),1.0);
        state.next("No invented extra stages");QCOMPARE(state.completed(),4);
    }
};
QTEST_GUILESS_MAIN(StartupTests)
#include "startup_tests.moc"
