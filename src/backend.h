#pragma once

#include <QHash>
#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QVector>

#include "filepicker.h"

class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString sourceFolderPath READ sourceFolderPath NOTIFY stateChanged)
    Q_PROPERTY(QString sourceFolderName READ sourceFolderName NOTIFY stateChanged)
    Q_PROPERTY(QString discardFolderPath READ discardFolderPath NOTIFY stateChanged)
    Q_PROPERTY(QString discardFolderName READ discardFolderName NOTIFY stateChanged)
    Q_PROPERTY(bool canStart READ canStart NOTIFY stateChanged)
    Q_PROPERTY(bool isSessionActive READ isSessionActive NOTIFY stateChanged)
    Q_PROPERTY(bool hasImages READ hasImages NOTIFY stateChanged)
    Q_PROPERTY(bool isFinished READ isFinished NOTIFY stateChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY stateChanged)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY stateChanged)
    Q_PROPERTY(int remainingCount READ remainingCount NOTIFY stateChanged)
    Q_PROPERTY(int keptCount READ keptCount NOTIFY stateChanged)
    Q_PROPERTY(int discardedCount READ discardedCount NOTIFY stateChanged)
    Q_PROPERTY(int movedCount READ movedCount NOTIFY stateChanged)
    Q_PROPERTY(int pendingMoveCount READ pendingMoveCount NOTIFY stateChanged)
    Q_PROPERTY(bool movesApplied READ movesApplied NOTIFY stateChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY stateChanged)
    Q_PROPERTY(QUrl currentUrl READ currentUrl NOTIFY stateChanged)
    Q_PROPERTY(QUrl nextImageUrl READ nextImageUrl NOTIFY stateChanged)
    Q_PROPERTY(bool currentIsVideo READ currentIsVideo NOTIFY stateChanged)
    Q_PROPERTY(QString currentName READ currentName NOTIFY stateChanged)
    Q_PROPERTY(QString currentResolution READ currentResolution NOTIFY stateChanged)
    Q_PROPERTY(QString currentSizeText READ currentSizeText NOTIFY stateChanged)
    Q_PROPERTY(QStringList buckets READ buckets NOTIFY stateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)

public:
    explicit Backend(FilePicker *picker = nullptr, QObject *parent = nullptr);

    QString sourceFolderPath() const { return m_sourceFolderPath; }
    QString sourceFolderName() const;
    QString discardFolderPath() const { return m_discardFolderPath; }
    QString discardFolderName() const;
    bool canStart() const { return !m_sourceFolderPath.isEmpty() && !m_discardFolderPath.isEmpty(); }
    bool isSessionActive() const { return m_sessionActive; }
    bool hasImages() const { return m_files.size() > 0; }
    bool isFinished() const { return m_sessionActive && !m_files.isEmpty() && m_index >= m_files.size(); }
    int currentIndex() const { return m_index; }
    int totalCount() const { return m_files.size(); }
    int remainingCount() const { return qMax(0, m_files.size() - m_index); }
    int keptCount() const { return m_keptCount; }
    int discardedCount() const { return m_discardedCount; }
    int movedCount() const { return m_movedCount; }
    int pendingMoveCount() const { return m_pendingMoves.size(); }
    bool movesApplied() const { return m_movesApplied; }
    bool canUndo() const { return !m_movesApplied && !m_undoStack.isEmpty(); }

    QUrl currentUrl() const;
    QUrl nextImageUrl() const;
    bool currentIsVideo() const;
    QString currentName() const;
    QString currentResolution() const;
    QString currentSizeText() const;
    QStringList buckets() const { return m_buckets; }
    QString errorMessage() const { return m_errorMessage; }

    Q_INVOKABLE void requestSourceFolder();
    Q_INVOKABLE void requestDiscardFolder();
    Q_INVOKABLE bool setSourceFolder(const QString &path);
    Q_INVOKABLE bool setDiscardFolder(const QString &path);
    Q_INVOKABLE bool startSifting();
    Q_INVOKABLE void resetSession();
    Q_INVOKABLE bool openFolder(const QString &path);
    Q_INVOKABLE void keep();
    Q_INVOKABLE void discard();
    Q_INVOKABLE bool createFolder(const QString &name);
    Q_INVOKABLE void moveToFolder(int bucketIndex);
    Q_INVOKABLE void undo();
    Q_INVOKABLE bool applyMoves();
    Q_INVOKABLE void copyCurrent();
    Q_INVOKABLE void clearError();

signals:
    void stateChanged();
    void errorChanged();
    void copied();

private:
    enum class ActionType { Kept, Discarded, Moved };

    struct UndoEntry {
        ActionType type;
        int index = 0;
        QString filePath;
    };

    void queueMoveToDir(const QString &targetDir, ActionType type);
    void setError(const QString &message);
    void refreshCurrentMetadata();
    static QString uniqueTargetPath(const QString &dirPath, const QString &fileName);
    static bool moveFile(const QString &src, const QString &dst);

    FilePicker *m_picker = nullptr;
    QString m_sourceFolderPath;
    QString m_discardFolderPath;
    bool m_customDiscardFolder = false;
    bool m_sessionActive = false;
    bool m_movesApplied = false;
    QString m_startFile;
    QStringList m_files;
    QStringList m_buckets;
    int m_index = 0;
    int m_keptCount = 0;
    int m_discardedCount = 0;
    int m_movedCount = 0;
    QVector<UndoEntry> m_undoStack;
    QHash<QString, QString> m_pendingMoves;
    QString m_currentResolution;
    QString m_currentSizeText;
    QString m_errorMessage;
};
