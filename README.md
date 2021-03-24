# Proftaak2.3_A1

# Instructions for menuconfig
Things to change in menuconfig :

(Default is set in sdkconfig.defaults)
- component config -> FAT Filesystem support
    - OEM Code Page = Simplified Chinese (DBCS) (CP936)
    - Long filename support = (Long filename buffer in heap)
    - API characer encoding = (API uses UTF-8 encoding)

- Wifi Connection Configuration (Used to connect to the internet)
    - Your own WIFI SSID 
    - Your own WIFI Password
