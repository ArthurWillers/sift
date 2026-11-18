#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QProcess>
#include <QTimer>

class Theme : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isDark READ isDark NOTIFY changed)
    Q_PROPERTY(QString background READ background NOTIFY changed)
    Q_PROPERTY(QString surface READ surface NOTIFY changed)
    Q_PROPERTY(QString elevated READ elevated NOTIFY changed)
    Q_PROPERTY(QString border READ border NOTIFY changed)
    Q_PROPERTY(QString foreground READ foreground NOTIFY changed)
    Q_PROPERTY(QString mutedForeground READ mutedForeground NOTIFY changed)
    Q_PROPERTY(QString accent READ accent NOTIFY changed)
    Q_PROPERTY(QString accentForeground READ accentForeground NOTIFY changed)
    Q_PROPERTY(QString keepColor READ keepColor NOTIFY changed)
    Q_PROPERTY(QString discardColor READ discardColor NOTIFY changed)
    Q_PROPERTY(int radius READ radius NOTIFY changed)

public:
    explicit Theme(const QString &currentDirectory = {}, QObject *parent = nullptr, const QString &hyprctl = {});
    ~Theme() override;

    bool isDark() const { return m_isDark; }
    QString background() const { return m_background; }
    QString surface() const { return m_surface; }
    QString elevated() const { return m_elevated; }
    QString border() const { return m_border; }
    QString foreground() const { return m_foreground; }
    QString mutedForeground() const { return m_mutedForeground; }
    QString accent() const { return m_accent; }
    QString accentForeground() const;
    QString keepColor() const { return m_keepColor; }
    QString discardColor() const { return m_discardColor; }
    int radius() const { return m_radius; }

    static QString readAccent(const QString &path);

signals:
    void changed();

private:
    void reload();
    void refreshRounding();

    QString m_directory;
    bool m_isDark = true;
    QString m_background = "#0e0e10";
    QString m_surface = "#16161a";
    QString m_elevated = "#1f1f24";
    QString m_border = "#27272a";
    QString m_foreground = "#f4f4f5";
    QString m_mutedForeground = "#a1a1aa";
    QString m_accent = "#FFD60A";
    QString m_keepColor = "#4ade80";
    QString m_discardColor = "#f87171";
    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
    QString m_hyprctl;
    int m_radius = 0;
    QProcess m_roundingProcess;
    QTimer m_roundingPoll, m_roundingTimeout;
};
