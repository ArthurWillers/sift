#pragma once

#include "filepicker.h"

class PortalFilePicker : public FilePicker {
    Q_OBJECT

public:
    explicit PortalFilePicker(QObject *parent = nullptr) : FilePicker(parent) {}
    void pickDirectory(const QString &title, Callback callback) override;
};
