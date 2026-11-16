// Copyright 2023 sekigon-gonnoc
// SPDX-License-Identifier: GPL-2.0-or-later

#define EMBEDDED_CLI_IMPL
#include "embedded_cli.h"

#include "debug.h"
#include "bootloader.h"
#include "pio_usb_ll.h"

extern void tusb_print_debug_buffer(void);

#define CLI_BUFFER_SIZE 1024
static CLI_UINT           cliBuffer[BYTES_TO_CLI_UINTS(CLI_BUFFER_SIZE)];
static EmbeddedCli       *cli             = NULL;
static bool               cli_initialized = false;
static EmbeddedCliConfig *config;
static volatile bool      isLoadMode = false;

void virtser_recv(uint8_t c) {
    embeddedCliReceiveChar(cli, c);
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
