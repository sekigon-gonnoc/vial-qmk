// Copyright 2023 sekigon-gonnoc
// SPDX-License-Identifier: GPL-2.0-or-later

#define EMBEDDED_CLI_IMPL
#include "embedded_cli.h"

#include "debug.h"
#include "bootloader.h"
#include "hardware/clocks.h"
#include "pio_usb_ll.h"
#if defined(USB_PACKET_CAPTURE_DEBUG) && USB_PACKET_CAPTURE_DEBUG
#include "usb_capture.h"
#include "virtser.h"
#endif

extern void tusb_print_debug_buffer(void);

#define CLI_BUFFER_SIZE 1024
static CLI_UINT           cliBuffer[BYTES_TO_CLI_UINTS(CLI_BUFFER_SIZE)];
static EmbeddedCli       *cli             = NULL;
static bool               cli_initialized = false;
static EmbeddedCliConfig *config;
static volatile bool      isLoadMode = false;

#if defined(USB_PACKET_CAPTURE_DEBUG) && USB_PACKET_CAPTURE_DEBUG
// Binary control protocol carried by virtser. Requests are
//   C7 51, command, sequence, payload_length_le16, payload
// Responses use command|0x80 and start their payload with a status byte.
// Commands: 1 ARM(auto bus speed), 2 FREEZE, 3 STATUS,
//           4 READ(offset_le32,length_le16), 5 CLEAR,
//           6 ACK_TRIGGER(enable_u8).
enum { CAPTURE_MAGIC0 = 0xc7, CAPTURE_MAGIC1 = 0x51, CAPTURE_MAX_REQUEST = 64,
       CAPTURE_MAX_READ = 48 };
enum { CAPTURE_OK = 0, CAPTURE_BAD_COMMAND = 1, CAPTURE_BAD_ARGUMENT = 2,
       CAPTURE_NOT_FROZEN = 3, CAPTURE_RANGE = 4, CAPTURE_UNAVAILABLE = 5 };
static uint8_t capture_rx[CAPTURE_MAX_REQUEST];
static uint8_t capture_rx_state;
static uint16_t capture_rx_len;
static uint16_t capture_rx_pos;
static uint8_t capture_rx_cmd, capture_rx_seq;

static void capture_send_frame(uint8_t command, uint8_t sequence,
                               const uint8_t *payload, uint16_t length) {
    virtser_send(CAPTURE_MAGIC0);
    virtser_send(CAPTURE_MAGIC1);
    virtser_send(command | 0x80);
    virtser_send(sequence);
    virtser_send((uint8_t)length);
    virtser_send((uint8_t)(length >> 8));
    for (uint16_t i = 0; i < length; i++) virtser_send(payload[i]);
}

static void capture_reply(uint8_t command, uint8_t sequence, uint8_t status,
                          const uint8_t *data, uint16_t data_length) {
    uint8_t payload[CAPTURE_MAX_READ + 1];
    if (data_length > CAPTURE_MAX_READ) data_length = CAPTURE_MAX_READ;
    payload[0] = status;
    for (uint16_t i = 0; i < data_length; i++) payload[i + 1] = data[i];
    capture_send_frame(command, sequence, payload, data_length + 1);
}

static uint32_t capture_u32le(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void capture_handle_request(void) {
    uint8_t response[CAPTURE_MAX_READ + 1];
    uint8_t status = CAPTURE_OK;
    uint16_t response_length = 0;
    usb_capture_status_t capture_status;

    switch (capture_rx_cmd) {
        case 1: // Arm / resume
            if (capture_rx_len != 0) status = CAPTURE_BAD_ARGUMENT;
            else if (!usb_capture_arm()) status = CAPTURE_UNAVAILABLE;
            break;
        case 2: // Freeze after the in-flight capture
            if (capture_rx_len != 0) status = CAPTURE_BAD_ARGUMENT;
            else usb_capture_freeze();
            break;
        case 3: // Status
            if (capture_rx_len != 0) {
                status = CAPTURE_BAD_ARGUMENT;
                break;
            }
            usb_capture_status(&capture_status);
            response[0] = capture_status.frozen ? 2 : capture_status.armed ? 1 : 0;
            response[1] = capture_status.low_speed ? 1 : 0;
            response[2] = capture_status.tail_incomplete ? 1 : 0;
            response[3] = (uint8_t)capture_status.sample_count;
            response[4] = (uint8_t)(capture_status.sample_count >> 8);
            response[5] = (uint8_t)(capture_status.sample_count >> 16);
            response[6] = (uint8_t)(capture_status.sample_count >> 24);
            response[7] = (uint8_t)capture_status.sample_rate_hz;
            response[8] = (uint8_t)(capture_status.sample_rate_hz >> 8);
            response[9] = (uint8_t)(capture_status.sample_rate_hz >> 16);
            response[10] = (uint8_t)(capture_status.sample_rate_hz >> 24);
            response[11] = (uint8_t)capture_status.data_size;
            response[12] = (uint8_t)(capture_status.data_size >> 8);
            response[13] = (uint8_t)(capture_status.data_size >> 16);
            response[14] = (uint8_t)(capture_status.data_size >> 24);
            response[15] = (uint8_t)capture_status.capacity;
            response[16] = (uint8_t)(capture_status.capacity >> 8);
            response[17] = (uint8_t)(capture_status.capacity >> 16);
            response[18] = (uint8_t)(capture_status.capacity >> 24);
            response[19] = capture_status.freeze_on_ack ? 1 : 0;
            response_length = 20;
            break;
        case 4: { // Read frozen bytes in bounded chunks
            if (capture_rx_len != 6) {
                status = CAPTURE_BAD_ARGUMENT;
                break;
            }
            usb_capture_status(&capture_status);
            uint32_t offset = capture_u32le(capture_rx);
            uint16_t length = (uint16_t)capture_rx[4] | ((uint16_t)capture_rx[5] << 8);
            if (!capture_status.frozen) status = CAPTURE_NOT_FROZEN;
            else if (length == 0 || length > CAPTURE_MAX_READ ||
                     offset > capture_status.data_size ||
                     length > capture_status.data_size - offset) status = CAPTURE_RANGE;
            else if (!usb_capture_read(offset, &response[1], length)) status = CAPTURE_RANGE;
            else response_length = length;
            break;
        }
        case 5: // Clear snapshot
            if (capture_rx_len != 0) status = CAPTURE_BAD_ARGUMENT;
            else usb_capture_clear();
            break;
        case 6: // Freeze after the next valid IN DATA+ACK transaction
            if (capture_rx_len != 1 || capture_rx[0] > 1) status = CAPTURE_BAD_ARGUMENT;
            else usb_capture_set_ack_trigger(capture_rx[0] != 0);
            break;
        default:
            status = CAPTURE_BAD_COMMAND;
            break;
    }

    if (capture_rx_cmd == 3 && status == CAPTURE_OK) {
        capture_reply(capture_rx_cmd, capture_rx_seq, status, response, response_length);
    } else if (capture_rx_cmd == 4 && status == CAPTURE_OK) {
        capture_reply(capture_rx_cmd, capture_rx_seq, status, &response[1], response_length);
    } else {
        capture_reply(capture_rx_cmd, capture_rx_seq, status, NULL, 0);
    }
}

static bool capture_protocol_receive(uint8_t c) {
    switch (capture_rx_state) {
        case 0:
            if (c == CAPTURE_MAGIC0) { capture_rx_state = 1; return true; }
            return false;
        case 1:
            if (c == CAPTURE_MAGIC1) { capture_rx_state = 2; capture_rx_pos = 0; return true; }
            capture_rx_state = (c == CAPTURE_MAGIC0) ? 1 : 0;
            if (cli) embeddedCliReceiveChar(cli, CAPTURE_MAGIC0);
            if (!capture_rx_state) return false;
            return true;
        case 2:
            capture_rx_cmd = c; capture_rx_state = 3; return true;
        case 3:
            capture_rx_seq = c; capture_rx_state = 4; return true;
        case 4:
            capture_rx_len = c; capture_rx_state = 5; return true;
        case 5:
            capture_rx_len |= (uint16_t)c << 8;
            capture_rx_pos = 0;
            if (capture_rx_len > CAPTURE_MAX_REQUEST) {
                capture_rx_state = 0;
                return true;
            }
            if (capture_rx_len == 0) {
                capture_rx_state = 0;
                capture_handle_request();
            } else capture_rx_state = 6;
            return true;
        default:
            capture_rx[capture_rx_pos++] = c;
            if (capture_rx_pos == capture_rx_len) {
                capture_rx_state = 0;
                capture_handle_request();
            }
            return true;
    }
}
#endif

void virtser_recv(uint8_t c) {
#if defined(USB_PACKET_CAPTURE_DEBUG) && USB_PACKET_CAPTURE_DEBUG
    if (capture_protocol_receive(c)) return;
#endif
    if (cli) embeddedCliReceiveChar(cli, c);
}

static void writeChar(EmbeddedCli *_, char c) { printf("%c", c); }

static void onVersion(EmbeddedCli *cli, char *args, void *context) {
    printf("firmware version: %s\n", STR(GIT_DESCRIBE));
}

static void onDfu(EmbeddedCli *cli, char *args, void *context) {
    bootloader_jump();
}

static void onDebug(EmbeddedCli *cli, char *args, void *context) {
    debug_enable = !debug_enable;
}

static void onPioUsbStatus(EmbeddedCli *cli, char *args, void *context) {
    pio_port_t const *pp = PIO_USB_PIO_PORT(0);
    printf("clk_sys: %lu Hz, FS TX divider: %u + %u/256\n",
           (unsigned long)clock_get_hz(clk_sys),
           (unsigned int)pp->clk_div_fs_tx.div_int,
           (unsigned int)pp->clk_div_fs_tx.div_frac);
    if (pp->total_transaction_count == 0) {
        printf("no USB transactions yet\n");
        return;
    }
    printf("error rate: %lu / %lu = %f%%\n",
           (unsigned long)pp->total_error_count,
           (unsigned long)pp->total_transaction_count,
           (float)pp->total_error_count / pp->total_transaction_count * 100);
    printf("fatal error rate: %lu / %lu = %f%%\n",
           (unsigned long)pp->total_fatal_error_count,
           (unsigned long)pp->total_transaction_count,
           (float)pp->total_fatal_error_count / pp->total_transaction_count * 100);
}

void cli_init(void) {
    config                     = embeddedCliDefaultConfig();
    config->cliBuffer          = cliBuffer;
    config->cliBufferSize      = CLI_BUFFER_SIZE;
    config->enableAutoComplete = false;
    config->historyBufferSize  = 64;
    config->maxBindingCount    = 16;
    cli                        = embeddedCliNew(config);
    cli->writeChar             = writeChar;
    embeddedCliAddBinding(
        cli, (CliCommandBinding){"version", "Show version", false, NULL,
                                 onVersion});
    embeddedCliAddBinding(
        cli, (CliCommandBinding){"piousb", "Pio usb status", false, NULL,
                                 onPioUsbStatus});
    embeddedCliAddBinding(cli, (CliCommandBinding){"dfu", "jump to bootloader",
                                                   false, NULL, onDfu});
    embeddedCliAddBinding(
        cli, (CliCommandBinding){"debug", "toggle debug option", false, NULL,
                                 onDebug});
    cli_initialized = true;
}

void cli_exec(void) {
    if (cli_initialized) embeddedCliProcess(cli);
    
    tusb_print_debug_buffer();
}
