#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QMimeData>
#include <QTemporaryDir>
#include <QtTest>

#include "backend.h"
#include "theme.h"
#include "translator.h"

namespace {
class FakePicker : public FilePicker {
public:
    QString nextPath;
    void pickDirectory(const QString &, Callback callback) override {
        callback(nextPath, {});
    }
};

void writeTestPng(const QString &path, const QColor &color = Qt::red) {
    QImage img(32, 24, QImage::Format_RGB32);
    img.fill(color);
    QVERIFY(img.save(path, "PNG"));
}
} // namespace

class SiftTests : public QObject {
    Q_OBJECT

private slots:
    void startsInSetupModeUntilFoldersChosen() {
        Backend backend;
        QVERIFY(!backend.isSessionActive());
        QVERIFY(!backend.canStart());
    }

    void customDiscardDestinationAndUndo() {
        QTemporaryDir srcDir;
        QTemporaryDir dstDir;
        writeTestPng(srcDir.filePath("a.png"), Qt::red);
        writeTestPng(srcDir.filePath("b.png"), Qt::blue);

        Backend backend;
        QVERIFY(backend.setSourceFolder(srcDir.path()));
        QVERIFY(backend.setDiscardFolder(dstDir.path()));
        QVERIFY(backend.canStart());
        QVERIFY(backend.startSifting());
        QVERIFY(backend.isSessionActive());
        QCOMPARE(backend.totalCount(), 2);

        backend.keep();
        QCOMPARE(backend.keptCount(), 1);
        QCOMPARE(backend.currentName(), QString("b.png"));

        backend.discard();
        QCOMPARE(backend.discardedCount(), 1);
        QCOMPARE(backend.pendingMoveCount(), 1);
        QVERIFY(backend.isFinished());
        // Nothing is moved on disk until applyMoves() is called.
        QVERIFY(QFileInfo::exists(srcDir.filePath("b.png")));
        QVERIFY(!QFileInfo::exists(dstDir.filePath("b.png")));

        backend.undo();
        QCOMPARE(backend.discardedCount(), 0);
        QCOMPARE(backend.pendingMoveCount(), 0);
        QVERIFY(!backend.isFinished());

        backend.discard();
        QCOMPARE(backend.pendingMoveCount(), 1);
        QVERIFY(backend.applyMoves());
        QVERIFY(backend.movesApplied());
        QCOMPARE(backend.pendingMoveCount(), 0);
        QVERIFY(!QFileInfo::exists(srcDir.filePath("b.png")));
        QVERIFY(QFileInfo::exists(dstDir.filePath("b.png")));
    }

    void createSubfolderAndMoveToBucket() {
        QTemporaryDir dir;
        writeTestPng(dir.filePath("shot.png"), Qt::yellow);

        Backend backend;
        QVERIFY(backend.openFolder(dir.path()));
        QVERIFY(backend.createFolder("Wallpapers"));
        QCOMPARE(backend.buckets(), QStringList{"Wallpapers"});

        backend.moveToFolder(0);
        QCOMPARE(backend.movedCount(), 1);
        QCOMPARE(backend.pendingMoveCount(), 1);
        QVERIFY(backend.isFinished());
        QVERIFY(QFileInfo::exists(dir.filePath("shot.png")));

        backend.undo();
        QCOMPARE(backend.movedCount(), 0);
        QCOMPARE(backend.pendingMoveCount(), 0);
        QVERIFY(!backend.isFinished());

        backend.moveToFolder(0);
        QVERIFY(backend.applyMoves());
        QVERIFY(QFileInfo::exists(dir.filePath("Wallpapers/shot.png")));
    }

    void rejectsInvalidSubfolderNames() {
        QTemporaryDir dir;
        Backend backend;
        QVERIFY(backend.openFolder(dir.path()));
        QVERIFY(!backend.createFolder(""));
        QVERIFY(!backend.createFolder("../escape"));
        QVERIFY(!backend.createFolder("_discarded"));
        QVERIFY(!backend.errorMessage().isEmpty());
    }

    void pickerSetsSourceAndDiscardFolders() {
        QTemporaryDir srcDir;
        QTemporaryDir dstDir;
        writeTestPng(srcDir.filePath("one.png"));

        FakePicker picker;
        Backend backend(&picker);

        picker.nextPath = srcDir.path();
        backend.requestSourceFolder();
        QCOMPARE(backend.sourceFolderPath(), srcDir.path());
        QCOMPARE(backend.discardFolderPath(), srcDir.path() + "/_discarded");

        picker.nextPath = dstDir.path();
        backend.requestDiscardFolder();
        QCOMPARE(backend.discardFolderPath(), dstDir.path());
        QVERIFY(!backend.isSessionActive());
    }

    void themeFallsBackAndReadsFullPalette() {
        QTemporaryDir dir;
        Theme fallback(dir.path());
        QCOMPARE(fallback.accent(), QString("#FFD60A"));
        QCOMPARE(fallback.isDark(), true);

        QDir(dir.path()).mkpath("theme");
        write(dir.path() + "/theme/colors.toml",
              "mode = \"light\"\n"
              "background = \"#f5f5f5\"\n"
              "dark_background = \"#eaeaea\"\n"
              "foreground = \"#111111\"\n"
              "accent = \"#112233\"\n"
              "bright_green = \"#228833\"\n"
              "bright_red = \"#cc2222\"\n");

        Theme theme(dir.path());
        QCOMPARE(theme.isDark(), false);
        QCOMPARE(theme.background(), QString("#f5f5f5"));
        QCOMPARE(theme.surface(), QString("#eaeaea"));
        QCOMPARE(theme.foreground(), QString("#111111"));
        QCOMPARE(theme.accent(), QString("#112233"));
        QCOMPARE(theme.accentForeground(), QString("white"));
        QCOMPARE(theme.keepColor(), QString("#228833"));
        QCOMPARE(theme.discardColor(), QString("#cc2222"));
    }

    void translatorFollowsLanguageFile() {
        QTemporaryDir dir;
        write(dir.path() + "/language", "pt-BR\n");
        write(dir.path() + "/pt.json", R"({"Keep": "Manter", "Choose": "Escolher"})");
        write(dir.path() + "/pt-BR.json", R"({"Choose": "Escolher pasta"})");

        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("pt-BR"));
        QCOMPARE(translator.translate("Main", "Choose"), QString("Escolher pasta"));
        QCOMPARE(translator.translate("Main", "Keep"), QString("Manter"));
        QVERIFY(translator.translate("Main", "Quit").isNull());
    }

    void recognizesVideosAndCopiesImageToClipboard() {
        QTemporaryDir dir;
        writeTestPng(dir.filePath("01_pic.png"), Qt::cyan);
        write(dir.filePath("02_clip.mp4"), "fake-mp4");

        Backend backend;
        QVERIFY(backend.openFolder(dir.path()));
        QCOMPARE(backend.totalCount(), 2);
        QVERIFY(!backend.currentIsVideo());

        backend.copyCurrent();
        const QMimeData *mime = QGuiApplication::clipboard()->mimeData();
        QVERIFY(mime != nullptr);
        QVERIFY(mime->hasUrls());
        QVERIFY(mime->hasImage());

        backend.keep();
        QVERIFY(backend.currentIsVideo());
    }

private:
    static void write(const QString &path, const QByteArray &contents) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(contents), contents.size());
    }
};

QTEST_MAIN(SiftTests)
#include "sift_tests.moc"
