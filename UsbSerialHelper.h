// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickView>
#include <QTimer>



class UsbSerialHelper {
public:
    struct SerialDevice {
        QString deviceName;
        QString driverName;
        int vendorId;
        int productId;
        int portCount;
    };

    UsbSerialHelper();

    static QList<SerialDevice> getAvailableDevices();

    // Optional: Get a specific driver by index
    static QJniObject getDriverAtIndex(int index);

    bool openDevice(int deviceIndex, int portIndex = 0, int baudRate = 9600);

    void closeDevice();

    QByteArray readData(int maxLength = 1024, int timeoutMs = 1000);

    bool writeData(const QByteArray &data, int timeoutMs = 1000);

signals:
    void permissionGranted();
    void permissionDenied();

private:
    QJniObject m_driver;
    QJniObject m_port;

    void requestPermission(const QJniObject &usbManager, const QJniObject &usbDevice);
};

