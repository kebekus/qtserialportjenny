// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickView>



class UsbSerialHelper {
public:
    struct SerialDevice {
        QString deviceName;
        QString driverName;
        int vendorId;
        int productId;
        int portCount;
    };

    static QList<SerialDevice> getAvailableDevices() {
        QList<SerialDevice> devices;

        // Get Android context
        auto *nativeInterface = QCoreApplication::instance()
            ->nativeInterface<QNativeInterface::QAndroidApplication>();
        
        if (!nativeInterface) {
            qWarning() << "Failed to get native interface";
            return devices;
        }

        QJniObject context = nativeInterface->context();
        if (!context.isValid()) {
            qWarning() << "Invalid context";
            return devices;
        }

        // Get UsbManager system service
        QJniObject usbServiceString = QJniObject::fromString("usb");
        QJniObject usbManager = context.callObjectMethod(
            "getSystemService",
            "(Ljava/lang/String;)Ljava/lang/Object;",
            usbServiceString.object()
        );

        if (!usbManager.isValid()) {
            qWarning() << "Failed to get UsbManager";
            return devices;
        }

        // Get default UsbSerialProber
        QJniObject prober = QJniObject::callStaticObjectMethod(
            "com/hoho/android/usbserial/driver/UsbSerialProber",
            "getDefaultProber",
            "()Lcom/hoho/android/usbserial/driver/UsbSerialProber;"
        );

        if (!prober.isValid()) {
            qWarning() << "Failed to get UsbSerialProber";
            return devices;
        }

        // Get list of available drivers
        QJniObject driverList = prober.callObjectMethod(
            "findAllDrivers",
            "(Landroid/hardware/usb/UsbManager;)Ljava/util/List;",
            usbManager.object()
        );

        if (!driverList.isValid()) {
            qWarning() << "Failed to get driver list";
            return devices;
        }

        // Get the size of the list
        int size = driverList.callMethod<jint>("size", "()I");
        qWarning() << "Found" << size << "USB serial device(s)";

        // Iterate through all drivers
        for (int i = 0; i < size; i++) {
            // Get driver at index i
            QJniObject driver = driverList.callObjectMethod(
                "get",
                "(I)Ljava/lang/Object;",
                i
            );

            if (!driver.isValid()) {
                continue;
            }

            SerialDevice device;

            // Get UsbDevice
            QJniObject usbDevice = driver.callObjectMethod(
                "getDevice",
                "()Landroid/hardware/usb/UsbDevice;"
            );

            if (usbDevice.isValid()) {
                // Get device name
                QJniObject deviceNameObj = usbDevice.callObjectMethod(
                    "getDeviceName",
                    "()Ljava/lang/String;"
                );
                device.deviceName = deviceNameObj.toString();

                // Get vendor ID
                device.vendorId = usbDevice.callMethod<jint>("getVendorId", "()I");

                // Get product ID
                device.productId = usbDevice.callMethod<jint>("getProductId", "()I");
            }

            // Get driver class name (e.g., CdcAcmSerialDriver, FtdiSerialDriver, etc.)
            QJniObject driverClass = driver.callObjectMethod(
                "getClass",
                "()Ljava/lang/Class;"
            );
            QJniObject driverClassName = driverClass.callObjectMethod(
                "getSimpleName",
                "()Ljava/lang/String;"
            );
            device.driverName = driverClassName.toString();

            // Get number of ports
            QJniObject ports = driver.callObjectMethod(
                "getPorts",
                "()Ljava/util/List;"
            );
            device.portCount = ports.callMethod<jint>("size", "()I");

            devices.append(device);

            // Log device info
            qWarning() << "Device" << i << ":";
            qWarning() << "  Name:" << device.deviceName;
            qWarning() << "  Driver:" << device.driverName;
            qWarning() << "  Vendor ID:" << QString("0x%1").arg(device.vendorId, 4, 16, QChar('0'));
            qWarning() << "  Product ID:" << QString("0x%1").arg(device.productId, 4, 16, QChar('0'));
            qWarning() << "  Port count:" << device.portCount;
        }

        return devices;
    }

    // Optional: Get a specific driver by index
    static QJniObject getDriverAtIndex(int index) {
        auto *nativeInterface = QCoreApplication::instance()
            ->nativeInterface<QNativeInterface::QAndroidApplication>();
        
        if (!nativeInterface) {
            return QJniObject();
        }

        QJniObject context = nativeInterface->context();
        QJniObject usbServiceString = QJniObject::fromString("usb");
        QJniObject usbManager = context.callObjectMethod(
            "getSystemService",
            "(Ljava/lang/String;)Ljava/lang/Object;",
            usbServiceString.object()
        );

        QJniObject prober = QJniObject::callStaticObjectMethod(
            "com/hoho/android/usbserial/driver/UsbSerialProber",
            "getDefaultProber",
            "()Lcom/hoho/android/usbserial/driver/UsbSerialProber;"
        );

        QJniObject driverList = prober.callObjectMethod(
            "findAllDrivers",
            "(Landroid/hardware/usb/UsbManager;)Ljava/util/List;",
            usbManager.object()
        );

        int size = driverList.callMethod<jint>("size", "()I");
        if (index < 0 || index >= size) {
            qWarning() << "Index out of range";
            return QJniObject();
        }

        return driverList.callObjectMethod(
            "get",
            "(I)Ljava/lang/Object;",
            index
        );
    }
};


int main(int argc, char *argv[])
{
    // In some cases Android app might not be able to safely clean all threads
    // while calling exit() and it might crash.
    // This flag avoids calling exit() and lets the Android system handle this,
    // at the cost of not attempting to run global destructors.
    qputenv("QT_ANDROID_NO_EXIT_CALL", "true");

    QGuiApplication app(argc, argv);




    // List all available USB serial devices
    QList<UsbSerialHelper::SerialDevice> devices = 
        UsbSerialHelper::getAvailableDevices();

    if (devices.isEmpty()) {
        qWarning() << "No USB serial devices found";
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
    }

    /*
    QQmlApplicationEngine engine;
    QObject::connect(
            &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
            []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("qtjenny_consumer", "Main");
*/

    return app.exec();
}
