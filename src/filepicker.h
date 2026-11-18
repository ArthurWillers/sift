#pragma once

#include <QObject>
#include <QString>
#include <functional>

class FilePicker : public QObject {
    Q_OBJECT

public:
    using Callback = std::function<void(const QString &path, const QString &error)>;
    explicit FilePicker(QObject *parent = nullptr) : QObject(parent) {}
    virtual void pickDirectory(const QString &title, Callback callback) = 0;
};
