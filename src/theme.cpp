// Theme watching follows Omacut (MIT, David Heinemeier Hansson).
#include "theme.h"

#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include <cmath>

namespace {
QHash<QString, QString> parseColorsToml(const QString &path) {
    QHash<QString, QString> values;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return values;

    const QString content = QString::fromUtf8(file.readAll());
    const QRegularExpression re(R"re(^\s*([a-zA-Z0-9_]+)\s*=\s*["']([^"']+)["'])re",
                                QRegularExpression::MultilineOption);
    auto it = re.globalMatch(content);
    while (it.hasNext()) {
        const auto match = it.next();
        values.insert(match.captured(1), match.captured(2));
    }
    return values;
}

double relativeLuminance(const QColor &c) {
    auto linear = [](double v) { return v <= .04045 ? v / 12.92 : std::pow((v + .055) / 1.055, 2.4); };
    return .2126 * linear(c.redF()) + .7152 * linear(c.greenF()) + .0722 * linear(c.blueF());
}
} // namespace

Theme::Theme(const QString &directory, QObject *parent, const QString &hyprctl)
    : QObject(parent),
      m_directory(directory.isEmpty() ? QDir::homePath() + "/.local/state/omarchy/current" : directory) {
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(80);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] { m_debounce.start(); });
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this] { m_debounce.start(); });
    connect(&m_debounce, &QTimer::timeout, this, &Theme::reload);

    m_hyprctl = !hyprctl.isEmpty() ? hyprctl
              : directory.isEmpty() && !qEnvironmentVariableIsEmpty("HYPRLAND_INSTANCE_SIGNATURE")
                  ? QStandardPaths::findExecutable("hyprctl") : QString();
    m_roundingTimeout.setSingleShot(true);
    m_roundingTimeout.setInterval(500);
    connect(&m_roundingTimeout, &QTimer::timeout, &m_roundingProcess, &QProcess::kill);
    connect(&m_roundingProcess, &QProcess::errorOccurred, this, [this] { m_roundingTimeout.stop(); });
    connect(&m_roundingProcess, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        m_roundingTimeout.stop();
        const auto value = QJsonDocument::fromJson(m_roundingProcess.readAllStandardOutput()).object().value("int");
        if (code != 0 || status != QProcess::NormalExit || !value.isDouble() || value.toInt(-1) < 0)
            return;
        if (value.toInt() != m_radius) {
            m_radius = value.toInt();
            emit changed();
        }
    });
    m_roundingPoll.setInterval(1000);
    connect(&m_roundingPoll, &QTimer::timeout, this, &Theme::refreshRounding);
    if (!m_hyprctl.isEmpty())
        m_roundingPoll.start();
    reload();
}

Theme::~Theme() {
    if (m_roundingProcess.state() != QProcess::NotRunning) {
        m_roundingProcess.kill();
        m_roundingProcess.waitForFinished(1000);
    }
}

void Theme::refreshRounding() {
    if (m_hyprctl.isEmpty() || m_roundingProcess.state() != QProcess::NotRunning)
        return;
    m_roundingProcess.start(m_hyprctl, {"-j", "getoption", "decoration:rounding"});
    m_roundingTimeout.start();
}

QString Theme::readAccent(const QString &path) {
    const auto values = parseColorsToml(path);
    const QString accent = values.value("accent");
    return QColor::isValidColorName(accent) ? accent : "#FFD60A";
}

QString Theme::accentForeground() const {
    return relativeLuminance(QColor(m_accent)) > .179 ? "black" : "white";
}

void Theme::reload() {
    refreshRounding();
    const auto paths = m_watcher.files() + m_watcher.directories();
    if (!paths.isEmpty())
        m_watcher.removePaths(paths);

    QString ancestor = m_directory;
    while (!QFileInfo::exists(ancestor) && ancestor != "/")
        ancestor = QFileInfo(ancestor).absolutePath();
    QStringList candidates{ancestor, QFileInfo(ancestor).absolutePath(), m_directory,
                           m_directory + "/theme", m_directory + "/theme/colors.toml"};
    candidates.removeDuplicates();
    for (const auto &path : candidates)
        if (QFileInfo::exists(path))
            m_watcher.addPath(path);

    const auto colors = parseColorsToml(m_directory + "/theme/colors.toml");
    auto pick = [&](const QStringList &keys, const QString &fallback) {
        for (const QString &k : keys) {
            const QString v = colors.value(k);
            if (!v.isEmpty() && QColor::isValidColorName(v))
                return v;
        }
        return fallback;
    };

    const QString background = pick({"background"}, "#0e0e10");
    const bool isDark = colors.contains("mode")
                            ? colors.value("mode") != "light"
                            : relativeLuminance(QColor(background)) <= .179;

    const QString surface = pick({"dark_background", "lighter_background"}, isDark ? "#16161a" : "#f4f4f5");
    const QString elevated = pick({"lighter_background", "selection"}, isDark ? "#1f1f24" : "#e4e4e7");
    const QString border = pick({"selection", "muted"}, isDark ? "#27272a" : "#d4d4d8");
    const QString foreground = pick({"foreground", "bright_foreground"}, isDark ? "#f4f4f5" : "#18181b");
    const QString mutedForeground = pick({"dark_foreground", "muted"}, isDark ? "#a1a1aa" : "#52525b");
    const QString accent = pick({"accent"}, "#FFD60A");
    const QString keepColor = pick({"bright_green", "green"}, "#4ade80");
    const QString discardColor = pick({"bright_red", "red"}, "#f87171");

    if (isDark != m_isDark || background != m_background || surface != m_surface ||
        elevated != m_elevated || border != m_border || foreground != m_foreground ||
        mutedForeground != m_mutedForeground || accent != m_accent ||
        keepColor != m_keepColor || discardColor != m_discardColor) {
        m_isDark = isDark;
        m_background = background;
        m_surface = surface;
        m_elevated = elevated;
        m_border = border;
        m_foreground = foreground;
        m_mutedForeground = mutedForeground;
        m_accent = accent;
        m_keepColor = keepColor;
        m_discardColor = discardColor;
        emit changed();
    }
}
