# If you are putting MicroPython on your board for the first time then you should first erase the entire flash using:

esptool.py --chip esp32s3 --port COM3 erase_flash
                  ^^^^^^^        ^^^^   
                  board          port

# From then on program the firmware starting at address 0:

esptool.py --chip esp32s3 --port COM3 write_flash -z 0 ESP32_GENERIC_S3-20251209-2-v1.27.0.bin 
                  ^^^^^^^        ^^^^                  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^    
                  board          port                  firmware (.bin file from MicroPython) 