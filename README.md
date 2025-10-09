# ESP32-S3 SmartWatch Project v3.0

A modern smartwatch implementation using ESP32-S3-Touch-LCD-2 development board with esp-brookesia UI framework for smartphone-style interface and BLE notifications.

## Features

- 📱 **Modern UI**: Smartphone-style interface using esp-brookesia library
- 🔔 **Phone Notifications**: Receive notifications via Bluetooth LE
- ⌚ **Watch Face**: Digital clock with date and battery display
- 📊 **Health Tracking**: Step counter using IMU sensor
- ⚙️ **Settings**: Brightness control and system configuration
- 🔋 **Power Management**: Optimized for battery operation
- 📡 **Wireless**: BLE connectivity for iOS and Android

## Hardware Requirements

### Development Board
- **ESP32-S3-Touch-LCD-2** (or compatible)
  - ESP32-S3 microcontroller
  - 240x320 IPS touchscreen display
  - 16MB Flash + 8MB PSRAM
  - QMI8658 6-axis IMU
  - Battery management circuit
  - USB-C for programming

### Additional Components (Optional)
- Li-Po battery (3.7V, 500-1000mAh)
- Vibration motor for haptic feedback
- Watch strap/case for wearable design

## Software Requirements

### Arduino IDE Setup

1. **Install Arduino IDE** (v2.0 or later)
   - Download from: https://www.arduino.cc/en/software

2. **Add ESP32 Board Support**
   - Open Arduino IDE
   - Go to File → Preferences
   - Add to "Additional Board Manager URLs":
     ```
     https://espressif.github.io/arduino-esp32/package_esp32_index.json
     ```
   - Go to Tools → Board → Board Manager
   - Search for "esp32" and install "esp32 by Espressif Systems"

3. **Install Required Libraries** via Library Manager:
   - **esp-brookesia** (v0.4.2+) - Modern UI framework
   - **ESP32_Display_Panel** (v0.2.0+) - Display hardware abstraction
   - **ESP32_IO_Expander** (v0.1.0+) - I/O expansion support
   - **lvgl** (v8.3.11) - Graphics library
   - **ArduinoJson** - JSON parsing for notifications

4. **Optional Libraries for Enhanced Features**:
   - **ESP32 BLE ANCS Notifications** - iOS notification support
   - **FastIMU** - Advanced IMU features

### Board Configuration

In Arduino IDE, select the following settings:

- **Board**: "ESP32S3 Dev Module"
- **USB CDC On Boot**: "Enabled"
- **CPU Frequency**: "240MHz (WiFi)"
- **Flash Mode**: "QIO 80MHz"
- **Flash Size**: "16MB (128Mb)"
- **Partition Scheme**: "Huge APP (3MB No OTA/1MB SPIFFS)"
- **Core Debug Level**: "None"
- **PSRAM**: "OPI PSRAM"
- **Arduino Runs On**: "Core 1"
- **Events Run On**: "Core 1"
- **Upload Speed**: "921600"
- **USB Mode**: "Hardware CDC and JTAG"

## Project Structure

```
SmartWatchV3/
├── SmartWatchV3.ino        # Main Arduino sketch
├── src/
│   ├── hardware_config.h   # Hardware pin definitions
│   ├── ble_notifications.h # BLE notification handling
│   ├── watch_apps.h        # Watch applications
│   ├── lvgl_port_v8.h      # LVGL configuration
│   └── lvgl_port_v8.cpp    # LVGL implementation
├── docs/
│   └── BUILD_GUIDE.md      # Detailed build instructions
└── README.md               # This file
```

## Installation Steps

1. **Clone or download this project** to your local machine

2. **Open SmartWatchV3.ino** in Arduino IDE

3. **Connect ESP32-S3-Touch-LCD-2** to your computer via USB-C

4. **Select the correct COM port** in Tools → Port

5. **Click Upload** to compile and flash the firmware

## First Run

After successful upload:

1. **The display will show** the esp-brookesia boot screen
2. **Watch face appears** with current time (needs RTC setup)
3. **Swipe gestures**:
   - Swipe up: App launcher
   - Swipe down: Notifications
   - Swipe left/right: Navigate between apps

## Bluetooth Pairing

### For Android:
1. Install a companion app that supports notification forwarding
2. Enable notification access in Android settings
3. Pair with "ESP32-Watch" in Bluetooth settings
4. Configure which apps can send notifications

### For iOS:
1. Pair with "ESP32-Watch" in Bluetooth settings
2. Allow notification access when prompted
3. iOS automatically sends notifications via ANCS

## Customization

### Adding New Apps

Create a new class inheriting from `ESP_Brookesia_PhoneApp`:

```cpp
class MyCustomApp : public ESP_Brookesia_PhoneApp {
public:
    MyCustomApp() : ESP_Brookesia_PhoneApp("MyApp", nullptr, false) {}
    
    void onResume() override {
        // Create your UI here
    }
    
    void onPause() override {
        // Cleanup when app goes to background
    }
};
```

Then install it in `installWatchApps()`:
```cpp
phone->installApp(new MyCustomApp());
```

### Changing Watch Face

Modify the `WatchFaceApp` class in `watch_apps.h` to customize:
- Time format (12/24 hour)
- Date format
- Additional widgets
- Background images

## Power Optimization Tips

1. **Reduce display brightness** when not actively viewing
2. **Use light sleep mode** between interactions
3. **Limit BLE advertising** frequency
4. **Disable unused features** (WiFi, etc.)
5. **Optimize refresh rates** for static content

## Memory Usage

With default configuration:
- **SRAM**: ~100KB used of 512KB
- **PSRAM**: ~500KB used of 8MB
- **Flash**: ~1.5MB used of 16MB

## Troubleshooting

### Display Issues
- Check pin connections in `hardware_config.h`
- Verify PSRAM is enabled in board settings
- Ensure sufficient power supply

### Touch Not Working
- Verify I2C connections (SDA/SCL)
- Check touch controller address (0x15)
- Test with simple touch example first

### BLE Connection Failed
- Ensure BLE is not disabled in settings
- Clear paired devices and retry
- Check if another device is connected

### Memory Errors
- Enable PSRAM in board configuration
- Reduce buffer sizes if needed
- Monitor memory usage in serial output

## Development Tips

1. **Use Serial Monitor** (115200 baud) for debugging
2. **Monitor memory usage** with the built-in reporting
3. **Test features incrementally** - don't enable everything at once
4. **Use FreeRTOS tasks** for parallel processing
5. **Keep UI updates in the main LVGL task**

## Resources

- [esp-brookesia Documentation](https://github.com/espressif/esp-brookesia)
- [LVGL Documentation](https://docs.lvgl.io/)
- [ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- [Arduino ESP32 Core](https://github.com/espressif/arduino-esp32)

## Known Limitations

- BLE and WiFi cannot be used simultaneously (ESP32 limitation)
- Maximum 10 concurrent apps due to memory constraints
- Touch gestures limited to basic swipes and taps
- No built-in GPS support (external module required)

## Future Enhancements

- [ ] Weather app with API integration
- [ ] Music control for phone
- [ ] Calendar sync
- [ ] Voice assistant integration
- [ ] Custom watch faces
- [ ] Sleep tracking
- [ ] Heart rate monitoring (requires external sensor)

## License

This project is provided as-is for educational purposes. Feel free to modify and use for personal projects.

## Contributing

Contributions are welcome! Please submit pull requests or issues on GitHub.

## Support

For questions and support:
- Check the troubleshooting section
- Review closed issues on GitHub
- Post in ESP32 community forums
- Contact the developer

---

**Version**: 3.0  
**Last Updated**: January 2025  
**Author**: SmartWatch Development Team
