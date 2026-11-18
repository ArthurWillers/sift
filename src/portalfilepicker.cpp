// PortalFilePicker follows Omacut (MIT, David Heinemeier Hansson).
#include "portalfilepicker.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QGuiApplication>
#include <QUrl>
#include <QVariantMap>
#include <QWindow>
#include <QUuid>

namespace {
class PortalRequest : public QObject {
    Q_OBJECT

public:
    PortalRequest(const QString &path, FilePicker::Callback callback, QObject *parent)
        : QObject(parent), m_path(path), m_callback(std::move(callback)) {
        QDBusConnection::sessionBus().connect(
            "org.freedesktop.portal.Desktop", m_path, "org.freedesktop.portal.Request", "Response",
            this, SLOT(onResponse(uint,QVariantMap)));
    }

public slots:
    void onResponse(uint response, const QVariantMap &results) {
        QDBusConnection::sessionBus().disconnect(
            "org.freedesktop.portal.Desktop", m_path, "org.freedesktop.portal.Request", "Response",
            this, SLOT(onResponse(uint,QVariantMap)));
        if (response == 0) {
            const QStringList uris = results.value("uris").toStringList();
            if (!uris.isEmpty())
                m_callback(QUrl(uris.first()).toLocalFile(), {});
            else
                m_callback({}, {});
        } else if (response == 1) {
            m_callback({}, {});
        } else {
            m_callback({}, tr("Folder picker failed."));
        }
        deleteLater();
    }

private:
    QString m_path;
    FilePicker::Callback m_callback;
};
} // namespace

void PortalFilePicker::pickDirectory(const QString &title, Callback callback) {
    const QString token = "sift_" + QUuid::createUuid().toString(QUuid::Id128);
    QVariantMap options;
    options.insert("handle_token", token);
    options.insert("directory", true);
    options.insert("modal", true);

    QString parentWindow;
    const auto windows = QGuiApplication::topLevelWindows();
    if (!windows.isEmpty() && windows.first()->winId() != 0)
        parentWindow = QString("x11:%1").arg(windows.first()->winId(), 0, 16);

    QDBusMessage msg = QDBusMessage::createMethodCall(
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.FileChooser",
        "OpenFile");
    msg << parentWindow << title << options;

    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(msg), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, callback = std::move(callback), watcher]() mutable {
        watcher->deleteLater();
        QDBusPendingReply<QDBusObjectPath> reply = *watcher;
        if (reply.isError()) {
            callback({}, tr("Cannot open folder picker: %1").arg(reply.error().message()));
            return;
        }
        new PortalRequest(reply.value().path(), std::move(callback), this);
    });
}

#include "portalfilepicker.moc"
