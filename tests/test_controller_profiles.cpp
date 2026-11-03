/*
    Copyright (c) 2015-2026, BogDan Vatra <bogdan@kde.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "controllerprofiles.h"
#include "controllerinput.h"

#include <QFile>
#include <QGamepadManager>
#include <QJsonDocument>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::StringLiterals;

class ProfilesTest : public QObject
{
    Q_OBJECT
    QTemporaryDir m_directory;
    QVariantMap mapping() const
    {
        QVariantList keys;
        for (int key : ControllerInput::qaopKeys) keys.append(key);
        return {{u"directionMode"_s, 1}, {u"directionKeys"_s, keys}, {u"joystick"_s, 2},
                {u"deadZone"_s, 0.25}, {u"fastForwardToggle"_s, true},
                {u"actions"_s, QVariantMap{{u"0"_s, Qt::Key_Space},
                                         {u"4"_s, int(ControllerAction::FastForward)}}}};
    }
    QUrl document(const QJsonObject &object)
    {
        const auto path = m_directory.filePath(u"profile.json"_s);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) qFatal("Cannot create test document");
        file.write(QJsonDocument(object).toJson());
        file.close();
        return QUrl::fromLocalFile(path);
    }
private slots:
    void initTestCase()
    {
        QVERIFY(m_directory.isValid());
        QCoreApplication::setOrganizationName(u"SpectacolTests"_s);
        QCoreApplication::setApplicationName(u"Profiles"_s);
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_directory.path());
    }
    void init() { QSettings().clear(); }
    void persistenceAndGameAssociations()
    {
        ControllerProfiles profiles;
        QVERIFY(profiles.save(u"QAOP"_s, mapping()));
        QVERIFY(profiles.save(u"Other"_s, mapping()));
        QCOMPARE(profiles.names(), (QStringList{u"Other"_s,u"QAOP"_s}));
        profiles.setGame(u"Dizzy 5"_s);
        QSignalSpy activation(&profiles, &ControllerProfiles::activationRequested);
        QVERIFY(profiles.assignGame(u"QAOP"_s));
        QCOMPARE(activation.size(), 1);
        ControllerProfiles restarted;
        restarted.setGame(u"dizzy 5"_s);
        QCOMPARE(restarted.gameProfile(), u"QAOP"_s);
        QCOMPARE(restarted.mapping(u"QAOP"_s), mapping());
        restarted.setGame(u"Another game"_s);
        QVERIFY(restarted.gameProfile().isEmpty());
        restarted.setGame(u"Dizzy 5"_s);
        QVERIFY(restarted.assignGame(QString{}));
        QVERIFY(restarted.gameProfile().isEmpty());
        QVERIFY(restarted.assignGame(u"QAOP"_s));
        QVERIFY(restarted.remove(u"QAOP"_s));
        QVERIFY(restarted.gameProfile().isEmpty());
        ControllerProfiles again;
        again.setGame(u"Dizzy 5"_s);
        QVERIFY(again.gameProfile().isEmpty());
        QVERIFY(!again.names().contains(u"QAOP"_s));
    }
    void snapshotNamesAndAliases()
    {
        ControllerProfiles profiles;
        QVERIFY(profiles.save(u"Keys"_s, mapping()));
        profiles.setGame(u"Dizzy.5.tap"_s);
        QCOMPARE(profiles.gameName(), u"Dizzy.5"_s);
        QVERIFY(profiles.assignGame(u"Keys"_s));
        profiles.setGame(u"Dizzy.5.2026-11-03_15_26_35_123"_s);
        QCOMPARE(profiles.gameProfile(), u"Keys"_s);
        QVERIFY(profiles.rememberGame(u"Before the boss"_s, u"Keys"_s));
        QCOMPARE(profiles.gameName(), u"Dizzy.5"_s); // Save As does not replace the current game
        ControllerProfiles restarted;
        restarted.setGame(u"Before the boss"_s);
        QCOMPARE(restarted.gameProfile(), u"Keys"_s);
    }
    void portableRoundTripAndConflicts()
    {
        ControllerProfiles profiles;
        QVERIFY(profiles.save(u"Custom keys"_s, mapping()));
        const auto url = QUrl::fromLocalFile(m_directory.filePath(u"export.json"_s));
        QVERIFY(profiles.exportProfile(u"Custom keys"_s, url));
        QCOMPARE(profiles.importProfile(url), u"Custom keys (2)"_s);
        QCOMPARE(profiles.mapping(u"Custom keys (2)"_s), mapping());
        QCOMPARE(profiles.importProfile(url), u"Custom keys (3)"_s);
        QVERIFY(profiles.remove(u"Custom keys"_s));
        QCOMPARE(profiles.importProfile(url), u"Custom keys"_s);
        QCOMPARE(profiles.mapping(u"Custom keys"_s), mapping());
        QVERIFY(!profiles.exportProfile(u"Missing"_s, url));
    }
    void malformedDocumentsNeverReplaceProfiles()
    {
        ControllerProfiles profiles;
        QVERIFY(profiles.save(u"Keep"_s, mapping()));
        const QJsonObject good{{u"format"_s,u"spectacol-controller-profile"_s}, {u"version"_s,1},
                               {u"name"_s,u"Keep"_s}, {u"mapping"_s,QJsonObject::fromVariantMap(mapping())}};
        for (const auto &key : {u"format"_s,u"version"_s,u"name"_s,u"mapping"_s}) {
            auto bad = good; bad.remove(key);
            QVERIFY(profiles.importProfile(document(bad)).isEmpty());
            QCOMPARE(profiles.mapping(u"Keep"_s), mapping());
        }
        auto bad = good; bad[u"version"_s] = 2;
        QVERIFY(profiles.importProfile(document(bad)).isEmpty());
        for (const auto &key : {u"directionMode"_s,u"directionKeys"_s,u"joystick"_s,u"deadZone"_s,u"actions"_s,u"fastForwardToggle"_s}) {
            auto invalid = mapping(); invalid.remove(key);
            QVERIFY(!profiles.save(u"Keep"_s, invalid));
            QCOMPARE(profiles.mapping(u"Keep"_s), mapping());
        }
        for (const auto &pair : {qMakePair(u"directionMode"_s,QVariant(1.5)), qMakePair(u"joystick"_s,QVariant(8)),
                                 qMakePair(u"deadZone"_s,QVariant(-0.1)), qMakePair(u"fastForwardToggle"_s,QVariant(1))}) {
            auto invalid = mapping(); invalid[pair.first] = pair.second;
            QVERIFY(!ControllerProfiles::validMapping(invalid));
        }
        auto invalid = mapping(); invalid[u"actions"_s] = QVariantMap{{QString::number(QGamepadManager::ButtonStart),0}};
        QVERIFY(!profiles.save(u"Keep"_s, invalid)); // Start remains reserved for navigation
        invalid[u"actions"_s] = QVariantMap{{u"0"_s,Qt::Key_F12}};
        QVERIFY(!profiles.save(u"Keep"_s, invalid));
        QVERIFY(!profiles.save(QString{}, mapping()));
        QVERIFY(!profiles.assignGame(u"Keep"_s));
        const auto path = m_directory.filePath(u"oversized.json"_s);
        QFile file(path); QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(QByteArray(1024*1024+1,' ')); file.close();
        QVERIFY(profiles.importProfile(QUrl::fromLocalFile(path)).isEmpty());
        QCOMPARE(profiles.mapping(u"Keep"_s), mapping());
    }
    void failedExportPreservesDestination()
    {
        ControllerProfiles profiles;
        QVERIFY(profiles.save(u"Keep"_s, mapping()));
        const auto path = m_directory.filePath(u"occupied"_s);
        QVERIFY(QDir().mkpath(path));
        QFile marker(path + u"/keep"_s); QVERIFY(marker.open(QIODevice::WriteOnly));
        marker.write("existing data"); marker.close();
        QVERIFY(!profiles.exportProfile(u"Keep"_s, QUrl::fromLocalFile(path)));
        QVERIFY(marker.open(QIODevice::ReadOnly));
        QCOMPARE(marker.readAll(), QByteArray("existing data"));
        QVERIFY(profiles.exportProfile(u"Keep"_s, QUrl::fromLocalFile(path + u"/valid.json"_s)));
    }
};
QTEST_GUILESS_MAIN(ProfilesTest)
#include "test_controller_profiles.moc"
