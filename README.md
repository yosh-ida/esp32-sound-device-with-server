# esp32-sound-device-with-server

## TODO
1. ~~AP->STAにして、SDカード上のSSID,PASSのリストをもとにAPを検索、接続してhttpサーバーを立てる。~~<br>
APmodeだと、sdカード内の構造検索が異様に遅くなっている。構造検索しないものは早いことから高速化(全構造解析、txtにhtmlを保存しておく)するとか。
2. multi threadでplayerがCPU1を占有しているが、バランスをよくして以下のエラー原因を探す。
`E (47525) task_wdt: Task watchdog got triggered. The following tasks did not reset the watchdog in time:
E (47525) task_wdt:  - IDLE0 (CPU 0)
E (47525) task_wdt: Tasks currently running:
E (47525) task_wdt: CPU 0: loop
E (47525) task_wdt: CPU 1: loopTask
E (47525) task_wdt: Aborting.
abort() was called at PC 0x400f3a63 on core 0``c: invoke_abort at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/esp32/panic.c line 155
0x4008ca0d: abort at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/esp32/panic.c line 170
0x400f3a63: task_wdt_isr at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/esp32/task_wdt.c line 174
0x4008a091: vTaskExitCritical at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/freertos/tasks.c line 4274
0x40088cd7: xQueueGenericReceive at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/freertos/queue.c line 1536
0x4014b6a2: sys_mutex_lock at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/lwip/port/esp32/freertos/sys_arch.c line 78
0x401398f9: lwip_recvfrom_r at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/lwip/lwip/src/api/sockets.c line 3400
0x4013996e: lwip_recv_r at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/lwip/lwip/src/api/sockets.c line 3406
0x400d2fd9: WiFiClient::connected() at C:\Users\Haruki\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.4/tools/sdk/include/lwip/lwip/sockets.h line 583
0x400d262a: serverProcess() at C:\Users\Haruki\Documents\Arduino\music_server/music_server.ino line 174
0x400d2897: multiLoop(void*) at C:\Users\Haruki\Documents\Arduino\music_server/music_server.ino line 88
0x40088f25: vPortTaskWrapper at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/freertos/port.c line 143`
3. telnetからのhttpリクエストを受け付けないことが多いことの検証。
