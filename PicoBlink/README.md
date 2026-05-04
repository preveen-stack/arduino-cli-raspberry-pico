arduino-cli compile --fqbn rp2040:rp2040:rpipico:freq=125 --upload -p /dev/cu.usbmodem14101 PicoBlink
Board name:                Raspberry Pi Pico
FQBN:                      rp2040:rp2040:rpipico
Board version:             5.6.0

Identification properties: pid=0x000a
                           vid=0x2e8a

Identification properties: pid=0x010a
                           vid=0x2e8a

Identification properties: pid=0x400a
                           vid=0x2e8a

Identification properties: pid=0x410a
                           vid=0x2e8a

Identification properties: pid=0x800a
                           vid=0x2e8a

Identification properties: pid=0x810a
                           vid=0x2e8a

Identification properties: pid=0xc00a
                           vid=0x2e8a

Identification properties: pid=0xc10a
                           vid=0x2e8a

Package name:              rp2040
Package maintainer:        Earle F. Philhower, III
Package URL:               https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
Package website:           https://github.com/earlephilhower/arduino-pico
Package online help:       https://arduino-pico.readthedocs.io/en/latest/

Platform name:             Raspberry Pi Pico/RP2040/RP2350
Platform category:         Raspberry Pi Pico
Platform architecture:     rp2040
Platform URL:              https://github.com/earlephilhower/arduino-pico/releases/download/5.6.0/rp2040-5.6.0.zip
Platform file name:        rp2040-5.6.0.zip
Platform size (bytes):     110079454
Platform checksum:         SHA-256:9fad6b92dfe25772346f8ef0132c918424e48b2b6ab9145f7d8df60c5cb43d30

Required tool: rp2040:pqt-gcc                     4.1.0-1aec55e
Required tool: rp2040:pqt-gcc-riscv               4.1.0-1aec55e
Required tool: rp2040:pqt-mklittlefs              4.1.0-1aec55e
Required tool: rp2040:pqt-openocd                 4.1.0-1aec55e
Required tool: rp2040:pqt-picotool                4.1.0-1aec55e
Required tool: rp2040:pqt-pioasm                  4.1.0-1aec55e
Required tool: rp2040:pqt-python3                 1.0.1-base-3a57aed-1

Option:        Flash Size                                              flash
               [32m2MB (no FS)[0m                        [32m✔[0m                    [32mflash=2097152_0[0m
               2MB (Sketch: 1984KB, FS: 64KB)                          flash=2097152_65536
               2MB (Sketch: 1920KB, FS: 128KB)                         flash=2097152_131072
               2MB (Sketch: 1792KB, FS: 256KB)                         flash=2097152_262144
               2MB (Sketch: 1536KB, FS: 512KB)                         flash=2097152_524288
               2MB (Sketch: 1MB, FS: 1MB)                              flash=2097152_1048576
Option:        CPU Speed                                               freq
               [32m200 MHz[0m                            [32m✔[0m                    [32mfreq=200[0m
               50 MHz                                                  freq=50
               100 MHz                                                 freq=100
               120 MHz                                                 freq=120
               125 MHz                                                 freq=125
               128 MHz                                                 freq=128
               133 MHz                                                 freq=133
               150 MHz                                                 freq=150
               176 MHz                                                 freq=176
               225 MHz (Overclock)                                     freq=225
               240 MHz (Overclock)                                     freq=240
               250 MHz (Overclock)                                     freq=250
               276 MHz (Overclock)                                     freq=276
               300 MHz (Overclock)                                     freq=300
Option:        Optimize                                                opt
               [32mSmall (-Os) (standard)[0m             [32m✔[0m                    [32mopt=Small[0m
               Optimize (-O)                                           opt=Optimize
               Optimize More (-O2)                                     opt=Optimize2
               Optimize Even More (-O3)                                opt=Optimize3
               Fast (-Ofast) (maybe slower)                            opt=Fast
               Debug (-Og)                                             opt=Debug
               Disabled (-O0)                                          opt=Disabled
Option:        Operating System                                        os
               [32mNone[0m                               [32m✔[0m                    [32mos=none[0m
               FreeRTOS SMP                                            os=freertos
Option:        Profiling                                               profile
               [32mDisabled[0m                           [32m✔[0m                    [32mprofile=Disabled[0m
               Enabled                                                 profile=Enabled
Option:        RTTI                                                    rtti
               [32mDisabled[0m                           [32m✔[0m                    [32mrtti=Disabled[0m
               Enabled                                                 rtti=Enabled
Option:        Stack Protector                                         stackprotect
               [32mDisabled[0m                           [32m✔[0m                    [32mstackprotect=Disabled[0m
               Enabled                                                 stackprotect=Enabled
Option:        C++ Exceptions                                          exceptions
               [32mDisabled[0m                           [32m✔[0m                    [32mexceptions=Disabled[0m
               Enabled                                                 exceptions=Enabled
Option:        Debug Port                                              dbgport
               [32mDisabled[0m                           [32m✔[0m                    [32mdbgport=Disabled[0m
               Serial                                                  dbgport=Serial
               Serial1                                                 dbgport=Serial1
               Serial2                                                 dbgport=Serial2
               SerialSemi                                              dbgport=SerialSemi
Option:        Debug Level                                             dbglvl
               [32mNone[0m                               [32m✔[0m                    [32mdbglvl=None[0m
               Core                                                    dbglvl=Core
               SPI                                                     dbglvl=SPI
               Wire                                                    dbglvl=Wire
               Bluetooth                                               dbglvl=Bluetooth
               BLE                                                     dbglvl=BLE
               LWIP                                                    dbglvl=LWIP
               All                                                     dbglvl=All
               NDEBUG                                                  dbglvl=NDEBUG
Option:        USB Stack                                               usbstack
               [32mPico SDK[0m                           [32m✔[0m                    [32musbstack=picosdk[0m
               Adafruit TinyUSB                                        usbstack=tinyusb
               Adafruit TinyUSB Host (native)                          usbstack=tinyusb_host
               No USB                                                  usbstack=nousb
Option:        IP/Bluetooth Stack                                      ipbtstack
               [32mIPv4 Only[0m                          [32m✔[0m                    [32mipbtstack=ipv4only[0m
               IPv4 + IPv6                                             ipbtstack=ipv4ipv6
               IPv4 + Bluetooth                                        ipbtstack=ipv4btcble
               IPv4 + IPv6 + Bluetooth                                 ipbtstack=ipv4ipv6btcble
               IPv4 Only - 32K                                         ipbtstack=ipv4onlybig
               IPv4 + IPv6 - 32K                                       ipbtstack=ipv4ipv6big
               IPv4 + Bluetooth - 32K                                  ipbtstack=ipv4btcblebig
               IPv4 + IPv6 + Bluetooth - 32K                           ipbtstack=ipv4ipv6btcblebig
Option:        Upload Method                                           uploadmethod
               [32mDefault (UF2)[0m                      [32m✔[0m                    [32muploadmethod=default[0m
               Picotool                                                uploadmethod=picotool
               Picoprobe/Debugprobe (CMSIS-DAP)                        uploadmethod=picoprobe_cmsis_dap
Programmers:   ID                                 Name

