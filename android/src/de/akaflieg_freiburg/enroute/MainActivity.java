// MainActivity.java
package org.qtproject.example.appqtjenny_consumer;

import android.content.Intent;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import org.qtproject.qt.android.bindings.QtActivity;

// MainActivity.java
public class MainActivity extends QtActivity {

    private static MainActivity instance = null;
    private static UsbDevice pendingDevice = null;
    private static boolean qtInitialized = false;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        instance = this;  // Store reference
        handleUsbIntent(getIntent());
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        instance = null;  // Clean up
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        handleUsbIntent(intent);
    }

    private void handleUsbIntent(Intent intent) {
        if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(intent.getAction())) {
            UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
            if (device != null && isSerialDevice(device)) {
                if (qtInitialized) {
                    notifyUsbDeviceAttached(device);
                } else {
                    pendingDevice = device;
                }
            }
        }
    }

    public static void onQtInitialized() {
        qtInitialized = true;
        if (pendingDevice != null && instance != null) {
            instance.notifyUsbDeviceAttached(pendingDevice);
            pendingDevice = null;
        }
    }

    private void notifyUsbDeviceAttached(UsbDevice device) {
        nativeNotifyUsbDevice(
            device.getDeviceName(),
            device.getVendorId(),
            device.getProductId(),
            getDriverName(device)
        );
    }

    private boolean isSerialDevice(UsbDevice device) {
        UsbManager usbManager = (UsbManager) getSystemService(USB_SERVICE);
        com.hoho.android.usbserial.driver.UsbSerialDriver driver =
            com.hoho.android.usbserial.driver.UsbSerialProber.getDefaultProber().probeDevice(device);
        return driver != null;
    }

    private String getDriverName(UsbDevice device) {
        com.hoho.android.usbserial.driver.UsbSerialDriver driver =
            com.hoho.android.usbserial.driver.UsbSerialProber.getDefaultProber().probeDevice(device);
        if (driver != null) {
            return driver.getClass().getSimpleName();
        }
        return "Unknown";
    }

    private native void nativeNotifyUsbDevice(String deviceName, int vendorId,
                                              int productId, String driverName);
}
