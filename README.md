# Proftaak2.3_A1

# Instructions for menuconfig
Things to change in menuconfig :

- component config -> FAT Filesystem support
    - OEM Code Page = Simplified Chinese (DBCS) (CP936)
    - Long filename support = (Long filename buffer in heap)
    - API characer encoding = (API uses UTF-8 encoding)

- Wifi Connection Configuration (Used to connect to the internet)
    - Your own WIFI SSID 
    - Your own WIFI Password

If you have problems with the memory of the ESP32 then change the following.
Go to $ENV{IDF_PATH}/components/partition_table and edit partitons_singleapp.
In the excel sheet of partitions_singleapp change the size of the factory from 1M to 0x180000.