// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickView>
#include <QTimer>

#include "UsbSerialHelper.h"


int main(int argc, char *argv[])
{
    // In some cases Android app might not be able to safely clean all threads
    // while calling exit() and it might crash.
    // This flag avoids calling exit() and lets the Android system handle this,
    // at the cost of not attempting to run global destructors.
    qputenv("QT_ANDROID_NO_EXIT_CALL", "true");

    QGuiApplication app(argc, argv);


    UsbSerialHelper helper;

    // List all available USB serial devices
    qDebug() << "=== Scanning for USB Serial Devices ===";
    QList<UsbSerialHelper::SerialDevice> devices =
        UsbSerialHelper::getAvailableDevices();

    if (devices.isEmpty()) {
        qDebug() << "No USB serial devices found";
    } else {
        qWarning() << "\nSummary:";
        for (int i = 0; i < devices.size(); i++) {
            const auto &device = devices[i];
            qWarning() << QString("[%1] %2 (%3) - VID:0x%4 PID:0x%5")
                              .arg(i)
                              .arg(device.deviceName)
                              .arg(device.driverName)
                              .arg(device.vendorId, 4, 16, QChar('0'))
                              .arg(device.productId, 4, 16, QChar('0'));
        }

        // Open the first device
        qDebug() << "\n=== Opening Device 0 ===";
        if (!helper.openDevice(0, 0, 9600)) {
            qDebug() << "Failed to open device - check permissions";
            // In a real app, you'd wait for permission callback and retry
            return 1;
        }

        // Write some data
        qDebug() << "\n=== Writing Data ===";
        QByteArray testData = "Hello USB!\r\n";
        if (helper.writeData(testData)) {
            qDebug() << "Successfully wrote:" << testData;
        }

        // Read data continuously for 10 seconds
        qDebug() << "\n=== Reading Data ===";
        QTimer *readTimer = new QTimer(&app);
        int readCount = 0;
        const int maxReads = 10;

        QObject::connect(readTimer, &QTimer::timeout, [&]() {
            QByteArray data = helper.readData(1024, 100);

            if (!data.isEmpty()) {
                qDebug() << "Received" << data.size() << "bytes:" << data;
            } else {
                qDebug() << "No data available";
            }

            readCount++;
            if (readCount >= maxReads) {
                readTimer->stop();
                helper.closeDevice();
                QCoreApplication::quit();
            }
        });

        readTimer->start(1000); // Read every second
    }


    QQmlApplicationEngine engine;
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("qtjenny_consumer", "Main");

    return app.exec();
}
