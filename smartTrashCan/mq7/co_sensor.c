#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <math.h>
#include <time.h>

// ====== 設定區 ======
#define SPI_DEVICE "/dev/spidev0.0"
#define SPI_SPEED 1000000 // 1MHz
#define VREF 3.3               // MCP3008參考電壓
#define ADC_RESOLUTION 1023.0  // MCP3008 10-bit解析度
#define RL_VALUE 10000.0       // MQ-7負載電阻（Ω）
#define RO_VALUE 10000.0       // 校正出的Ro（示範10kΩ）
#define ALERT_THRESHOLD_PPM 200.0 // 超標警戒線 (ppm)
#define ALERT_HOLD_TIME 60         // 超標持續秒數 (秒)
//#define HTML_FILE_PATH "~/smartTrashCan/status/co_status.txt" // 寫入HTML資料位置
// ====================

int spi_fd = -1;
char html_file_path[512]; //宣告環境變數(開一個變數存路徑)

// ====== 函式宣告 ======
void update_html(double co_ppm, int danger);
void init_html_path();

// 初始化 HTML 寫入路徑
void init_html_path() {
    const char *home = getenv("HOME");
    if (home == NULL) {
        printf("Error: HOME environment variable not set!\n");
        exit(1);
    }
    snprintf(html_file_path, sizeof(html_file_path), "%s/smartTrashCan/status/co_status.txt", home);
    printf("📁 將寫入 CO 狀態檔案：%s\n", html_file_path);
}

// 初始化 SPI
int spi_init() {
    spi_fd = open(SPI_DEVICE, O_RDWR);
    if (spi_fd < 0) {
        perror("SPI: Can't open device");
        return -1;
    }

    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;
    uint32_t speed = SPI_SPEED;

    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) == -1 ||
        ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1 ||
        ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1) {
        perror("SPI: Failed to set SPI parameters");
        close(spi_fd);
        return -1;
    }

    return 0;
}

void spi_close() {
    if (spi_fd >= 0) close(spi_fd);
}

// 讀 MCP3008
uint16_t read_adc_channel(uint8_t channel) {
    if (channel > 7) return 0;

    uint8_t tx[] = { 0x01, (0x08 | channel) << 4, 0x00 };
    uint8_t rx[3] = {0};

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 3,
        .delay_usecs = 0,
        .speed_hz = SPI_SPEED,
        .bits_per_word = 8,
    };

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 1) {
        perror("SPI: Failed to send SPI message");
        return 0;
    }

    return ((rx[1] & 0x03) << 8) | rx[2];
}

// 計算 ppm
double get_ppm(double rs_ro_ratio) {
    double a = 99.042;
    double b = -1.518;
    return a * pow(rs_ro_ratio, b);
}

// 寫入 CO 狀態檔案
void update_html(double co_ppm, int danger) {
    printf("📝 update_html 寫入中... PPM: %.1f, DANGER: %d\n", co_ppm, danger);
    FILE *fp = fopen(html_file_path, "w");
    if (fp == NULL) {
        perror("Failed to open CO status file");
        return;
    }

    time_t now = time(NULL);

    if (co_ppm > ALERT_THRESHOLD_PPM) {
        fprintf(fp, "ALERT\n%.1f\n%ld\n", co_ppm, now);
        if (danger) {
            fprintf(fp, "DANGER\n");
        }
    } else {
        fprintf(fp, "NORMAL\n%.1f\n%ld\n", co_ppm, now);
    }

    fclose(fp);
}

int main() {
    init_html_path();

    if (spi_init() != 0) return -1;

    uint8_t channel = 0;
    time_t first_alert_time = 0;
    int has_written_initial = 0;
	int last_danger_written = 0;

    printf("=== CO濃度即時監測開始 ===\n");

	while (1) {
    	uint16_t adc_value = read_adc_channel(channel);
	    double vout = (adc_value / ADC_RESOLUTION) * VREF;
	    double rs = RL_VALUE * (VREF - vout) / vout;
	    double rs_ro_ratio = rs / RO_VALUE;
	    double co_ppm = get_ppm(rs_ro_ratio);
	    time_t now = time(NULL);

	    printf("[Time: %ld] ADC: %u | V: %.3f V | Rs: %.1f Ω | Rs/Ro: %.3f | CO: %.1f ppm\n",
    	    now, adc_value, vout, rs, rs_ro_ratio, co_ppm);

    	// 第一次就寫入
    	if (!has_written_initial) {
        	update_html(co_ppm, 0);
        	has_written_initial = 1;
    	}

    	if (co_ppm > ALERT_THRESHOLD_PPM) {
        	if (first_alert_time == 0) {
            	first_alert_time = now;
            	printf("⚠️ 開始計時超標...\n");
        	} else if (now - first_alert_time >= ALERT_HOLD_TIME) {
            	printf("🚨 超標超過 %d 秒，寫入警告！\n", ALERT_HOLD_TIME);
            	update_html(co_ppm, 1);  // ← 有 danger
				last_danger_written = 1;
            	first_alert_time = 0;
        	}
    	} else {
        	if (first_alert_time != 0 || last_danger_written) {
            	printf("✅ 超標解除，寫回正常狀態！\n");
            	update_html(co_ppm, 0);  // ← 正常狀態
            	first_alert_time = 0;
				last_danger_written = 0;
        	}
    	}

	    // 濃度正常的話，每秒也寫一次讓前端看到更新
		update_html(co_ppm, last_danger_written);
    	sleep(1);
	}

    spi_close();
    return 0;
}
