// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickView>
#include <QTimer>

#include "UsbSerialHelper.h"

UsbSerialHelper::UsbSerialHelper()
{
    QJniObject::callStaticMethod<void>(
        "org/qtproject/example/appqtjenny_consumer/MainActivity",
        "onQtInitialized",
        "()V"
        );
}


QList<UsbSerialHelper::SerialDevice> UsbSerialHelper::getAvailableDevices() {
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
QJniObject UsbSerialHelper::getDriverAtIndex(int index) {
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


bool UsbSerialHelper::openDevice(int deviceIndex, int portIndex, int baudRate) {
    auto *nativeInterface = QCoreApplication::instance()
    ->nativeInterface<QNativeInterface::QAndroidApplication>();

    if (!nativeInterface) {
        qWarning() << "Failed to get native interface";
        return false;
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
    if (deviceIndex < 0 || deviceIndex >= size) {
        qWarning() << "Device index out of range";
        return false;
    }

    // Get the driver
    m_driver = driverList.callObjectMethod(
        "get",
        "(I)Ljava/lang/Object;",
        deviceIndex
        );

    if (!m_driver.isValid()) {
        qWarning() << "Invalid driver";
        return false;
    }

    // Get the USB device
    QJniObject usbDevice = m_driver.callObjectMethod(
        "getDevice",
        "()Landroid/hardware/usb/UsbDevice;"
        );

    // Check if we have permission
    bool hasPermission = usbManager.callMethod<jboolean>(
        "hasPermission",
        "(Landroid/hardware/usb/UsbDevice;)Z",
        usbDevice.object()
        );

    if (!hasPermission) {
        qWarning() << "No USB permission - requesting...";
        requestPermission(usbManager, usbDevice);
        return false; // Will need to retry after permission granted
    }

    // Get the port
    QJniObject ports = m_driver.callObjectMethod(
        "getPorts",
        "()Ljava/util/List;"
        );

    int portCount = ports.callMethod<jint>("size", "()I");
    if (portIndex < 0 || portIndex >= portCount) {
        qWarning() << "Port index out of range";
        return false;
    }

    m_port = ports.callObjectMethod(
        "get",
        "(I)Ljava/lang/Object;",
        portIndex
        );

    if (!m_port.isValid()) {
        qWarning() << "Invalid port";
        return false;
    }

    // Open a connection to the USB device
    QJniObject connection = usbManager.callObjectMethod(
        "openDevice",
        "(Landroid/hardware/usb/UsbDevice;)Landroid/hardware/usb/UsbDeviceConnection;",
        usbDevice.object()
        );

    if (!connection.isValid()) {
        qWarning() << "Failed to open USB connection";
        return false;
    }

    // Open the serial port
    QJniEnvironment env;
    m_port.callMethod<void>(
        "open",
        "(Landroid/hardware/usb/UsbDeviceConnection;)V",
        connection.object()
        );

    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        qWarning() << "Failed to open serial port";
        return false;
    }

    // Set parameters: baud rate, data bits, stop bits, parity
    m_port.callMethod<void>(
        "setParameters",
        "(IIII)V",
        baudRate,     // baud rate
        8,            // data bits (8)
        1,            // stop bits (1 = STOPBITS_1)
        0             // parity (0 = PARITY_NONE)
        );

    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        qWarning() << "Failed to set port parameters";
        closeDevice();
        return false;
    }

    qDebug() << "Successfully opened device" << deviceIndex
             << "port" << portIndex
             << "at" << baudRate << "baud";

    return true;
}

void UsbSerialHelper::closeDevice() {
    if (m_port.isValid()) {
        QJniEnvironment env;
        m_port.callMethod<void>("close", "()V");

        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }

        m_port = QJniObject();
        qDebug() << "Device closed";
    }
    m_driver = QJniObject();
}

QByteArray UsbSerialHelper::readData(int maxLength, int timeoutMs) {
    if (!m_port.isValid()) {
        qWarning() << "Port not open";
        return QByteArray();
    }

    QJniEnvironment env;

    // Create a byte array to read into
    jbyteArray buffer = env->NewByteArray(maxLength);

    // Read from the port
    int bytesRead = m_port.callMethod<jint>(
        "read",
        "([BI)I",
        buffer,
        timeoutMs
        );

    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->DeleteLocalRef(buffer);
        return QByteArray();
    }

    if (bytesRead <= 0) {
        env->DeleteLocalRef(buffer);
        return QByteArray();
    }

    // Convert Java byte array to QByteArray
    jbyte* bytes = env->GetByteArrayElements(buffer, nullptr);
    QByteArray data(reinterpret_cast<const char*>(bytes), bytesRead);
    env->ReleaseByteArrayElements(buffer, bytes, JNI_ABORT);
    env->DeleteLocalRef(buffer);

    return data;
}

bool UsbSerialHelper::writeData(const QByteArray &data, int timeoutMs)
{
    if (!m_port.isValid()) {
        qWarning() << "Port not open";
        return false;
    }

    QJniEnvironment env;

    // Create Java byte array from QByteArray
    jbyteArray buffer = env->NewByteArray(data.size());
    env->SetByteArrayRegion(buffer, 0, data.size(),
                            reinterpret_cast<const jbyte*>(data.constData()));

    // Write to the port
    int bytesWritten = m_port.callMethod<jint>(
        "write",
        "([BI)I",
        buffer,
        timeoutMs
        );

    env->DeleteLocalRef(buffer);

    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return false;
    }

    if (bytesWritten != data.size()) {
        qWarning() << "Only wrote" << bytesWritten << "of" << data.size() << "bytes";
        return false;
    }

    qDebug() << "Wrote" << bytesWritten << "bytes";
    return true;
}

void UsbSerialHelper::requestPermission(const QJniObject &usbManager, const QJniObject &usbDevice) {
    auto *nativeInterface = QCoreApplication::instance()
    ->nativeInterface<QNativeInterface::QAndroidApplication>();
    QJniObject context = nativeInterface->context();

    // Create a PendingIntent for the permission request
    QJniObject action = QJniObject::fromString("com.yourapp.USB_PERMISSION");

    QJniObject pendingIntent = QJniObject::callStaticObjectMethod(
        "android/app/PendingIntent",
        "getBroadcast",
        "(Landroid/content/Context;ILandroid/content/Intent;I)Landroid/app/PendingIntent;",
        context.object(),
        0,
        QJniObject("android/content/Intent", "(Ljava/lang/String;)V", action.object()).object(),
        0x04000000  // PendingIntent.FLAG_IMMUTABLE
        );

    // Request permission
    usbManager.callMethod<void>(
        "requestPermission",
        "(Landroid/hardware/usb/UsbDevice;Landroid/app/PendingIntent;)V",
        usbDevice.object(),
        pendingIntent.object()
        );

    qDebug() << "USB permission requested";
}



extern "C" {

JNIEXPORT void JNICALL
Java_org_qtproject_example_appqtjenny_1consumer_UsbConnectionReceiver_notifyUsbDeviceAttached(
    JNIEnv *env, jobject obj, jstring jDeviceName,
    jint vendorId, jint productId, jint deviceClass)
{
    const char *deviceName = env->GetStringUTFChars(jDeviceName, nullptr);

    qDebug() << "USB Device Attached:" << deviceName
             << "VID:" << QString::number(vendorId, 16)
             << "PID:" << QString::number(productId, 16);

/*
    UsbEventHandler::instance()->onDeviceAttached(
        QString(deviceName), vendorId, productId, deviceClass
        );
 */
    env->ReleaseStringUTFChars(jDeviceName, deviceName);
}

JNIEXPORT void JNICALL
Java_org_qtproject_example_appqtjenny_1consumer_UsbConnectionReceiver_notifyUsbDeviceDetached(
    JNIEnv *env, jobject obj, jstring jDeviceName)
{
    const char *deviceName = env->GetStringUTFChars(jDeviceName, nullptr);

    qDebug() << "USB Device Detached:" << deviceName;

    //UsbEventHandler::instance()->onDeviceDetached(QString(deviceName));

    env->ReleaseStringUTFChars(jDeviceName, deviceName);
}

JNIEXPORT void JNICALL
Java_org_qtproject_example_appqtjenny_1consumer_UsbConnectionReceiver_notifyAppStartedByUsbDevice(
    JNIEnv *env, jobject obj, jstring jDeviceName,
    jint vendorId, jint productId, jstring jDriverName)
{
    const char *deviceName = env->GetStringUTFChars(jDeviceName, nullptr);
    const char *driverName = env->GetStringUTFChars(jDriverName, nullptr);

    qDebug() << "App started by USB device:" << deviceName
             << "VID:" << QString::number(vendorId, 16)
             << "PID:" << QString::number(productId, 16)
             << "Driver:" << driverName;

/*
    // Notify your application logic
    UsbEventHandler::instance()->onAppStartedByDevice(
        QString(deviceName), vendorId, productId, QString(driverName)
        );
*/
    env->ReleaseStringUTFChars(jDeviceName, deviceName);
    env->ReleaseStringUTFChars(jDriverName, driverName);
}

} // extern "C"
