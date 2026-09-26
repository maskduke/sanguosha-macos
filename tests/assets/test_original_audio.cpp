#include <QtTest>
#include <QCryptographicHash>
#include <QDirIterator>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryFile>
#include <vorbis/vorbisfile.h>

// This validates source assets and libvorbisfile, not the playback backend.
class OriginalAudioTest : public QObject {
    Q_OBJECT
private slots:
    void originalOgg_data() {
        QTest::addColumn<QString>("relativePath");
        QTest::addColumn<QByteArray>("sha256");
        QFile manifest(QS_MANIFEST);
        QVERIFY(manifest.open(QIODevice::ReadOnly));
        QJsonParseError error;
        const auto document = QJsonDocument::fromJson(manifest.readAll(), &error);
        QCOMPARE(error.error, QJsonParseError::NoError);
        const auto entries = document.object()["entries"].toObject();
        const QString prefix = "Contents/MacOS/";
        QStringList expectedPaths;
        for (auto it = entries.begin(); it != entries.end(); ++it) {
            if (!it.key().startsWith(prefix) || !it.key().endsWith(".ogg")) continue;
            const QString path = it.key().mid(prefix.size());
            expectedPaths.append(path);
            const QByteArray hash = it.value().toObject()["sha256"].toString().toLatin1();
            QCOMPARE(hash.size(), 64);
            QTest::newRow(path.toUtf8().constData()) << path << hash;
        }
        QCOMPARE(expectedPaths.size(), 477);
        QStringList actualPaths;
        const QDir root(QS_ASSET_ROOT);
        QDirIterator files(root.absolutePath(), {"*.ogg"}, QDir::Files, QDirIterator::Subdirectories);
        while (files.hasNext()) actualPaths.append(root.relativeFilePath(files.next()));
        expectedPaths.sort();
        actualPaths.sort();
        QCOMPARE(actualPaths, expectedPaths);
    }
    void originalOgg() {
        QFETCH(QString, relativePath);
        QFETCH(QByteArray, sha256);
        const QString path = QDir(QS_ASSET_ROOT).filePath(relativePath);
        auto hashFile = [&]() {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly)) return QByteArray();
            QCryptographicHash hash(QCryptographicHash::Sha256);
            if (!hash.addData(&file)) return QByteArray();
            return hash.result().toHex();
        };
        QCOMPARE(hashFile(), sha256);
        OggVorbis_File stream{};
        QCOMPARE(ov_fopen(QFile::encodeName(path).constData(), &stream), 0);
        struct Close { OggVorbis_File *s; ~Close() { ov_clear(s); } } close{&stream};
        QVERIFY(ov_seekable(&stream));
        const ogg_int64_t declared = ov_pcm_total(&stream, -1);
        QVERIFY(declared > 0);
        ogg_int64_t decoded = 0;
        char buffer[65536];
        int section = 0;
        for (;;) {
            const long size = ov_read(&stream, buffer, sizeof buffer, 0, 2, 1, &section);
            QVERIFY2(size >= 0, "Vorbis decode error / hole in original OGG");
            if (size == 0) break;
            const auto *info = ov_info(&stream, section);
            QVERIFY(info);
            QVERIFY(info->channels > 0);
            QVERIFY(info->rate > 0);
            QCOMPARE(size % (2 * info->channels), 0L);
            decoded += size / (2 * info->channels);
        }
        const auto *info = ov_info(&stream, 0);
        QVERIFY(info);
        const QJsonObject observation{{"path", relativePath}, {"sha256", QString::fromLatin1(sha256)},
            {"frames", double(decoded)}, {"declared_frames", double(declared)}, {"streams", double(ov_streams(&stream))},
            {"first_stream_rate", double(info->rate)}, {"first_stream_channels", info->channels},
            {"seconds", ov_time_total(&stream, -1)}};
        qInfo().noquote() << QJsonDocument(observation).toJson(QJsonDocument::Compact);
        QCOMPARE(hashFile(), sha256);
        QCOMPARE(decoded, declared);
    }
    void rejectsNonVorbis() {
        QTemporaryFile file;
        QVERIFY(file.open());
        QVERIFY(file.write("not an ogg stream") > 0);
        QVERIFY(file.flush());
        OggVorbis_File stream{};
        const int result = ov_fopen(QFile::encodeName(file.fileName()).constData(), &stream);
        if (result == 0) ov_clear(&stream);
        QVERIFY(result < 0);
    }
};
QTEST_APPLESS_MAIN(OriginalAudioTest)
#include "test_original_audio.moc"
