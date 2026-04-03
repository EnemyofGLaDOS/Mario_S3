/* Arduino Nofrendo - StickS3
 * ROM loaded from rom_data.h (embedded in flash)
 * Please check hw_config.h and display.cpp for configuration details
 */

#include <M5Unified.h>
#include <esp_wifi.h>
#include <esp_task_wdt.h>
#include "hw_config.h"
#include "rom_data.h"

extern "C" {
#include <nofrendo.h>
}

extern void display_begin();

#include <SPIFFS.h>
#include <pgmspace.h>

#define TEMP_ROM_PATH "/spiffs/rom.nes"

static void dumpFileHeader(const char* path) {
    File rf = SPIFFS.open(path, "r");
    if (!rf) {
        Serial.println("DIAG: could not reopen ROM for readback");
        return;
    }
    Serial.printf("DIAG: SPIFFS ROM size = %u\n", (unsigned)rf.size());
    Serial.print("DIAG: first 16 bytes = ");
    for (int i = 0; i < 16 && rf.available(); i++) {
        uint8_t b = rf.read();
        Serial.printf("%02X ", b);
    }
    Serial.println();
    rf.close();
}

void setup()
{
    Serial.begin(115200);

    esp_wifi_deinit();

    display_begin();

    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS mount failed!");
        return;
    }

    Serial.printf("ROM size: %u bytes\n", rom_size);

    Serial.println("Writing embedded ROM to SPIFFS...");
    File f = SPIFFS.open("/rom.nes", "w");
    if (!f) {
        Serial.println("Failed to open ROM file for writing!");
        Serial.println("ROM write failed!");
        return;
    }

    const uint32_t CHUNK = 512;
    uint32_t written = 0;
    while (written < rom_size) {
        uint32_t toWrite = min(CHUNK, rom_size - written);
        uint8_t buf[CHUNK];
        for (uint32_t i = 0; i < toWrite; i++) {
            buf[i] = pgm_read_byte(rom_data + written + i);
        }
        size_t actuallyWritten = f.write(buf, toWrite);
        if (actuallyWritten != toWrite) {
            Serial.printf("DIAG: short write at %u, wanted %u got %u\n", written, toWrite, (unsigned)actuallyWritten);
            f.close();
            return;
        }
        written += toWrite;
    }
    f.close();
    Serial.printf("ROM written: %u bytes to %s\n", written, TEMP_ROM_PATH);
    dumpFileHeader(TEMP_ROM_PATH);

    char *argv[1];
    char romPath[] = TEMP_ROM_PATH;
    argv[0] = romPath;

    Serial.println("NoFrendo start!");
    int rc = nofrendo_main(1, argv);
    Serial.printf("NoFrendo returned: %d\n", rc);
    Serial.println("NoFrendo end!");
}

void loop()
{
}
