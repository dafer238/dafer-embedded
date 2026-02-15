# Migration Cleanup

This document explains how to clean up old files after verifying the refactored code works correctly.

## ⚠️ Warning

**Do NOT delete these files until you have:**
1. Successfully built the project (`idf.py build`)
2. Flashed to hardware and verified functionality
3. Confirmed all sensors are reading correctly
4. Made a backup of the working code

## Files to Remove (After Testing)

Once you've confirmed the refactored code works, you can safely remove these old files:

### In `main/` directory:

```bash
# Old C sensor drivers (replaced by components/)
rm main/bmp280.c
rm main/bmp280.h
rm main/dht22.c
rm main/dht22.h

# Old main application (replaced by app.cpp)
rm main/main.c
```

## Cleanup Commands

### Windows PowerShell
```powershell
# Navigate to project directory
cd ESP32\meteo_publisher

# Remove old sensor files
Remove-Item main\bmp280.c
Remove-Item main\bmp280.h
Remove-Item main\dht22.c
Remove-Item main\dht22.h

# Remove old main file
Remove-Item main\main.c

# Verify they're gone
Get-ChildItem main\*.c
```

### Linux/macOS
```bash
# Navigate to project directory
cd ESP32/meteo_publisher

# Remove old sensor files
rm main/bmp280.c main/bmp280.h
rm main/dht22.c main/dht22.h

# Remove old main file
rm main/main.c

# Verify they're gone
ls main/*.c
```

## Files to Keep

These files are part of the refactored architecture:

### Components (NEW)
- `components/bmp280/bmp280.c` ✅
- `components/bmp280/bmp280.h` ✅
- `components/bmp280/CMakeLists.txt` ✅
- `components/dht22/dht22.c` ✅
- `components/dht22/dht22.h` ✅
- `components/dht22/CMakeLists.txt` ✅

### Main Application (NEW)
- `main/app.cpp` ✅
- `main/SensorInterface.hpp` ✅
- `main/BMP280Sensor.hpp` ✅
- `main/BMP280Sensor.cpp` ✅
- `main/DHT22Sensor.hpp` ✅
- `main/DHT22Sensor.cpp` ✅

### Main Application (UNCHANGED)
- `main/wifi.c` ✅
- `main/wifi.h` ✅
- `main/mqtt_pub.c` ✅
- `main/mqtt_pub.h` ✅
- `main/led.c` ✅
- `main/led.h` ✅
- `main/Kconfig` ✅
- `main/CMakeLists.txt` ✅ (updated)

## Verification Steps Before Cleanup

### 1. Build Test
```bash
idf.py build
# Should complete successfully with no errors
```

### 2. Flash Test
```bash
idf.py flash monitor
# Should flash and run successfully
```

### 3. Sensor Verification
Check the serial monitor output for:
- ✅ "BMP280 detected"
- ✅ "BMP280Sensor wrapper initialized"
- ✅ "DHT22 initialized"
- ✅ "DHT22Sensor wrapper initialized"
- ✅ Temperature and pressure readings
- ✅ Humidity readings
- ✅ MQTT publish success

### 4. Functionality Checklist
- [ ] WiFi connects successfully
- [ ] BMP280 reads temperature and pressure
- [ ] DHT22 reads temperature and humidity
- [ ] MQTT publishes data
- [ ] LED blinks correctly
- [ ] Deep sleep activates
- [ ] Device wakes up and repeats cycle

## Rollback Procedure

If something goes wrong AFTER cleanup:

### Option 1: Git Revert (if using version control)
```bash
git checkout HEAD -- main/bmp280.c main/bmp280.h main/dht22.c main/dht22.h main/main.c
git checkout HEAD -- main/CMakeLists.txt
```

### Option 2: Manual Restore
1. Copy the old files from backup
2. Restore the old `main/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS
        "main.c"
        "wifi.c"
        "mqtt_pub.c"
        "dht22.c"
        "bmp280.c"
        "led.c"
    INCLUDE_DIRS "."
)
```
3. Remove or rename new files temporarily
4. Rebuild: `idf.py fullclean && idf.py build`

## Post-Cleanup Verification

After removing old files:

```bash
# Clean build to ensure no stale references
idf.py fullclean

# Build fresh
idf.py build

# Should complete successfully
# Flash and verify one more time
idf.py flash monitor
```

## File Size Comparison

Approximate sizes (for reference):

**Before cleanup:**
- `main/` directory: ~50 KB (includes duplicate sensor code)

**After cleanup:**
- `main/` directory: ~35 KB (C++ wrappers and app only)
- `components/` directory: ~25 KB (pure C drivers)
- **Total is similar, but better organized**

## Keeping Old Files (Optional)

If you want to keep old files for reference:

```bash
# Create archive directory
mkdir -p archive/original

# Move old files instead of deleting
mv main/bmp280.c main/bmp280.h archive/original/
mv main/dht22.c main/dht22.h archive/original/
mv main/main.c archive/original/
```

## FAQ

**Q: Can I run both old and new code simultaneously?**
A: No, they will conflict. Choose one or the other.

**Q: What if I only want to clean up one sensor?**
A: You can do incremental cleanup, but update CMakeLists.txt accordingly.

**Q: Will removing old files affect my git history?**
A: No, git preserves history. Old versions are still accessible.

**Q: Can I restore old files later?**
A: Yes, from git history or from backup.

## Recommended Approach

1. ✅ **Build and test** new code thoroughly
2. ✅ **Make backup** of entire project directory
3. ✅ **Commit to git** (if using version control)
4. ✅ **Remove old files** as documented above
5. ✅ **Clean build** and verify again
6. ✅ **Deploy to production** if all tests pass

## Support

If you encounter issues during cleanup:
1. Check `ARCHITECTURE.md` for design details
2. Review `REFACTORING.md` for migration guide
3. Consult `QUICK_REFERENCE.md` for common patterns
4. Restore from backup if needed
