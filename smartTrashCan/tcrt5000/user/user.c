#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int main() {
    char path[512];
    const char *home = getenv("HOME");
    if (home == NULL) {
        fprintf(stderr, "❌ 環境變數 HOME 沒有設，無法取得家目錄！\n");
        return 1;
    }
    snprintf(path, sizeof(path), "%s/smartTrashCan/status/tcrt_status.txt", home);

    while (1) {
        FILE *fp = fopen("/sys/class/tcrt5000_irq_driver/tcrt5000_sensor/tcrt5000_status", "r");
        if (fp == NULL) {
            perror("Failed to open sensor status");
            return 1;
        }

        int value;
        fscanf(fp, "%d", &value);
        fclose(fp);

        printf("Sensor Value: %d\n", value);

        // 傳送資料給 server
        char cmd[512];
        sprintf(cmd,
            "curl -s -X POST http://192.168.51.72:3000/api/tcrt/update -H \"Content-Type: application/json\" -d '{\"sensor\":%d}'",
            value);
        system(cmd);

        // 寫入狀態 + 時間戳
        FILE *status_fp = fopen(path, "w");
        if (status_fp) {
            time_t now = time(NULL);
            fprintf(status_fp, "%s\n%ld\n", value == 0 ? "FULL" : "NORMAL", now);
            fclose(status_fp);
        } else {
            perror("Failed to write tcrt_status.txt");
        }

        sleep(5);
    }

    return 0;
}

