#include "backend.h"

#include <QClipboard>
#include <QCollator>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QImageReader>
#include <QLocale>
#include <QMimeData>
#include <QSet>

namespace {
const QSet<QString> &imageExtensions() {
    static const QSet<QString> exts{
        "png", "jpg", "jpeg", "webp", "gif", "bmp", "avif", "svg", "tif", "tiff"
    };
    return exts;
}

const QSet<QString> &videoExtensions() {
    static const QSet<QString> exts{
        "mp4", "mov", "mkv", "webm", "avi", "m4v"
    };
    return exts;
}

bool isVideoFile(const QString &path) {
    return videoExtensions().contains(QFileInfo(path).suffix().toLower());
}

constexpr auto kDefaultDiscardSubdir = "_discarded";
} // namespace

Backend::Backend(FilePicker *picker, QObject *parent)
    : QObject(parent), m_picker(picker) {}

QString Backend::sourceFolderName() const {
    return m_sourceFolderPath.isEmpty() ? QString() : QFileInfo(m_sourceFolderPath).fileName();
}

QString Backend::discardFolderName() const {
    if (m_discardFolderPath.isEmpty())
        return {};
    const QFileInfo info(m_discardFolderPath);
    if (!m_sourceFolderPath.isEmpty() && info.absolutePath() == m_sourceFolderPath)
        return info.fileName();
    return m_discardFolderPath;
}

QUrl Backend::currentUrl() const {
    if (!m_sessionActive || m_index < 0 || m_index >= m_files.size())
        return {};
    return QUrl::fromLocalFile(m_files.at(m_index));
}

QUrl Backend::nextImageUrl() const {
    if (!m_sessionActive || m_index + 1 < 0 || m_index + 1 >= m_files.size())
        return {};
    const QString &nextPath = m_files.at(m_index + 1);
    if (isVideoFile(nextPath))
        return {};
    return QUrl::fromLocalFile(nextPath);
}

bool Backend::currentIsVideo() const {
    if (!m_sessionActive || m_index < 0 || m_index >= m_files.size())
        return false;
    return isVideoFile(m_files.at(m_index));
}

QString Backend::currentName() const {
    if (!m_sessionActive || m_index < 0 || m_index >= m_files.size())
        return {};
    return QFileInfo(m_files.at(m_index)).fileName();
}

QString Backend::currentResolution() const {
    return m_currentResolution;
}

QString Backend::currentSizeText() const {
    return m_currentSizeText;
}

void Backend::requestSourceFolder() {
    if (!m_picker) {
        setError(tr("No folder picker available."));
        return;
    }
    m_picker->pickDirectory(tr("Choose source folder"), [this](const QString &path, const QString &error) {
        if (!error.isEmpty()) {
            setError(error);
            return;
        }
        if (!path.isEmpty())
            setSourceFolder(path);
    });
}

void Backend::requestDiscardFolder() {
    if (!m_picker) {
        setError(tr("No folder picker available."));
        return;
    }
    m_picker->pickDirectory(tr("Choose discard folder"), [this](const QString &path, const QString &error) {
        if (!error.isEmpty()) {
            setError(error);
            return;
        }
        if (!path.isEmpty())
            setDiscardFolder(path);
    });
}

bool Backend::setSourceFolder(const QString &path) {
    clearError();
    const QFileInfo info(path);
    if (!info.exists()) {
        setError(tr("Folder does not exist: %1").arg(path));
        return false;
    }

    const QString targetDir = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
    QDir dir(targetDir);
    if (!dir.isReadable()) {
        setError(tr("Cannot read folder: %1").arg(targetDir));
        return false;
    }

    m_sourceFolderPath = targetDir;
    m_startFile = info.isFile() ? info.absoluteFilePath() : QString();
    if (!m_customDiscardFolder || m_discardFolderPath.isEmpty())
        m_discardFolderPath = targetDir + "/" + kDefaultDiscardSubdir;

    emit stateChanged();
    return true;
}

bool Backend::setDiscardFolder(const QString &path) {
    clearError();
    const QString trimmed = path.trimmed();
    if (trimmed.isEmpty()) {
        setError(tr("Invalid folder name."));
        return false;
    }
    const QFileInfo info(trimmed);
    m_discardFolderPath = info.absoluteFilePath();
    m_customDiscardFolder = true;
    emit stateChanged();
    return true;
}

bool Backend::startSifting() {
    clearError();
    if (m_sourceFolderPath.isEmpty()) {
        setError(tr("Choose a source folder first."));
        return false;
    }
    if (m_discardFolderPath.isEmpty()) {
        setError(tr("Choose a discard folder first."));
        return false;
    }
    if (QFileInfo(m_sourceFolderPath).absoluteFilePath() == QFileInfo(m_discardFolderPath).absoluteFilePath()) {
        setError(tr("Discard folder must be different from source folder."));
        return false;
    }

    QDir dir(m_sourceFolderPath);
    if (!dir.isReadable()) {
        setError(tr("Cannot read folder: %1").arg(m_sourceFolderPath));
        return false;
    }

    QStringList files;
    const QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Readable, QDir::NoSort);
    for (const QFileInfo &entry : entries) {
        const QString ext = entry.suffix().toLower();
        if (imageExtensions().contains(ext) || videoExtensions().contains(ext))
            files.append(entry.absoluteFilePath());
    }

    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(files.begin(), files.end(), collator);

    const QString discardAbs = QFileInfo(m_discardFolderPath).absoluteFilePath();
    QStringList buckets;
    const QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &sub : subdirs) {
        const QString subAbs = QFileInfo(m_sourceFolderPath + "/" + sub).absoluteFilePath();
        if (sub.startsWith('.') || sub == kDefaultDiscardSubdir || subAbs == discardAbs)
            continue;
        buckets.append(sub);
    }

    m_files = files;
    m_buckets = buckets;
    m_index = 0;
    if (!m_startFile.isEmpty()) {
        const int idx = m_files.indexOf(m_startFile);
        if (idx >= 0)
            m_index = idx;
    }
    m_keptCount = 0;
    m_discardedCount = 0;
    m_movedCount = 0;
    m_undoStack.clear();
    m_pendingMoves.clear();
    m_movesApplied = false;
    m_sessionActive = true;

    refreshCurrentMetadata();
    emit stateChanged();
    return true;
}

void Backend::resetSession() {
    clearError();
    m_sessionActive = false;
    m_movesApplied = false;
    m_files.clear();
    m_buckets.clear();
    m_index = 0;
    m_keptCount = 0;
    m_discardedCount = 0;
    m_movedCount = 0;
    m_undoStack.clear();
    m_pendingMoves.clear();
    m_currentResolution.clear();
    m_currentSizeText.clear();
    emit stateChanged();
}

bool Backend::openFolder(const QString &path) {
    if (!setSourceFolder(path))
        return false;
    return startSifting();
}

void Backend::keep() {
    if (!m_sessionActive || m_index < 0 || m_index >= m_files.size())
        return;
    clearError();
    m_undoStack.append({ActionType::Kept, m_index, m_files.at(m_index)});
    ++m_keptCount;
    ++m_index;
    refreshCurrentMetadata();
    emit stateChanged();
}

void Backend::discard() {
    if (m_discardFolderPath.isEmpty()) {
        setError(tr("Choose a discard folder first."));
        return;
    }
    queueMoveToDir(m_discardFolderPath, ActionType::Discarded);
}

bool Backend::createFolder(const QString &name) {
    clearError();
    if (m_sourceFolderPath.isEmpty())
        return false;

    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty() || trimmed.contains('/') || trimmed.contains('\\') || trimmed == "." || trimmed == "..") {
        setError(tr("Invalid folder name."));
        return false;
    }
    if (trimmed == kDefaultDiscardSubdir) {
        setError(tr("That name is reserved for discarded files."));
        return false;
    }

    QDir dir(m_sourceFolderPath);
    if (!dir.exists(trimmed) && !dir.mkdir(trimmed)) {
        setError(tr("Could not create folder: %1").arg(trimmed));
        return false;
    }

    if (!m_buckets.contains(trimmed)) {
        m_buckets.append(trimmed);
        emit stateChanged();
    }
    return true;
}

void Backend::moveToFolder(int bucketIndex) {
    if (bucketIndex < 0 || bucketIndex >= m_buckets.size())
        return;
    queueMoveToDir(m_sourceFolderPath + "/" + m_buckets.at(bucketIndex), ActionType::Moved);
}

void Backend::undo() {
    if (m_movesApplied || m_undoStack.isEmpty())
        return;
    clearError();

    const UndoEntry entry = m_undoStack.takeLast();
    if (entry.type == ActionType::Kept) {
        m_keptCount = qMax(0, m_keptCount - 1);
    } else {
        m_pendingMoves.remove(entry.filePath);
        if (entry.type == ActionType::Discarded)
            m_discardedCount = qMax(0, m_discardedCount - 1);
        else
            m_movedCount = qMax(0, m_movedCount - 1);
    }

    m_index = qBound(0, entry.index, m_files.size());
    refreshCurrentMetadata();
    emit stateChanged();
}

bool Backend::applyMoves() {
    if (!m_sessionActive || m_movesApplied)
        return false;
    clearError();

    if (m_pendingMoves.isEmpty()) {
        m_movesApplied = true;
        emit stateChanged();
        return true;
    }

    QStringList failed;
    QStringList succeeded;

    for (auto it = m_pendingMoves.cbegin(); it != m_pendingMoves.cend(); ++it) {
        const QString &src = it.key();
        const QString &targetDir = it.value();
        if (!QDir().mkpath(targetDir)) {
            failed.append(QFileInfo(src).fileName());
            continue;
        }
        const QString dst = uniqueTargetPath(targetDir, QFileInfo(src).fileName());
        if (!moveFile(src, dst)) {
            failed.append(QFileInfo(src).fileName());
            continue;
        }
        succeeded.append(src);
    }

    for (const QString &src : succeeded)
        m_pendingMoves.remove(src);

    if (!failed.isEmpty()) {
        setError(tr("Could not move %n file(s).", "", failed.size()));
        emit stateChanged();
        return false;
    }

    m_movesApplied = true;
    emit stateChanged();
    return true;
}

void Backend::copyCurrent() {
    if (!m_sessionActive || m_index < 0 || m_index >= m_files.size())
        return;
    clearError();

    const QString path = m_files.at(m_index);
    auto *mime = new QMimeData();
    mime->setUrls({QUrl::fromLocalFile(path)});
    if (!isVideoFile(path)) {
        const QImage image(path);
        if (!image.isNull())
            mime->setImageData(image);
    }
    QGuiApplication::clipboard()->setMimeData(mime);
    emit copied();
}

void Backend::clearError() {
    if (!m_errorMessage.isEmpty()) {
        m_errorMessage.clear();
        emit errorChanged();
    }
}

void Backend::queueMoveToDir(const QString &targetDir, ActionType type) {
    if (!m_sessionActive || m_movesApplied || m_index < 0 || m_index >= m_files.size())
        return;
    clearError();

    const QString src = m_files.at(m_index);
    m_pendingMoves.insert(src, targetDir);
    m_undoStack.append({type, m_index, src});
    ++m_index;
    if (type == ActionType::Discarded)
        ++m_discardedCount;
    else
        ++m_movedCount;

    refreshCurrentMetadata();
    emit stateChanged();
}

void Backend::setError(const QString &message) {
    if (m_errorMessage != message) {
        m_errorMessage = message;
        emit errorChanged();
    }
}

void Backend::refreshCurrentMetadata() {
    if (!m_sessionActive || m_index < 0 || m_index >= m_files.size()) {
        m_currentResolution.clear();
        m_currentSizeText.clear();
        return;
    }
    const QString path = m_files.at(m_index);
    const QFileInfo info(path);
    m_currentSizeText = QLocale().formattedDataSize(info.size());

    if (isVideoFile(path)) {
        m_currentResolution = tr("Video");
        return;
    }

    QImageReader reader(path);
    const QSize sz = reader.size();
    m_currentResolution = sz.isValid() ? QString("%1 × %2").arg(sz.width()).arg(sz.height()) : QString();
}

QString Backend::uniqueTargetPath(const QString &dirPath, const QString &fileName) {
    const QFileInfo info(fileName);
    const QString base = info.completeBaseName();
    const QString ext = info.suffix().isEmpty() ? QString() : "." + info.suffix();

    QString candidate = dirPath + "/" + fileName;
    int counter = 1;
    while (QFileInfo::exists(candidate)) {
        candidate = QString("%1/%2_%3%4").arg(dirPath, base).arg(counter++).arg(ext);
    }
    return candidate;
}

bool Backend::moveFile(const QString &src, const QString &dst) {
    if (QFile::rename(src, dst))
        return true;
    if (QFile::copy(src, dst)) {
        if (QFile::remove(src))
            return true;
        QFile::remove(dst);
    }
    return false;
}
