# USB packet capture virtser protocol

The optional protocol is enabled with `USB_PACKET_CAPTURE_DEBUG`. Ordinary
virtser input is still sent to the CLI; capture frames begin with the binary
magic bytes `C7 51`.

## Frame

All integers are little-endian. A request is:

```text
C7 51 command sequence payload_length_u16 payload...
```

A response uses the same sequence and `command | 0x80`. Its payload starts with
one status byte, followed by operation data on success. Status values are:

| Value | Meaning |
| --- | --- |
| 0 | Success |
| 1 | Unknown command |
| 2 | Wrong payload length or value |
| 3 | Capture is not frozen |
| 4 | Read range is invalid |

## Commands

| ID | Request payload | Successful response data |
| --- | --- | --- |
| 1 Arm / resume | none; waits for the next non-SOF host token and uses that bus speed | none |
| 2 Freeze | none | none |
| 3 Status | none | state:u8, speed:u8, tail_incomplete:u8, sample_count:u32, sample_rate_hz:u32, data_size:u32, capacity:u32, freeze_on_ack:u8 |
| 4 Read | offset:u32, length:u16 | up to 48 capture bytes |
| 5 Clear | none | none |
| 6 ACK trigger | enable:u8 | none |

Status state is `0` idle, `1` armed, or `2` frozen. `tail_incomplete` is `1`
when the DMA capture stopped on a transfer word boundary rather than a USB
end-of-packet boundary. `sample_count` counts raw
sample instants; four 2-bit D+/D− samples are packed into each capture byte.
`data_size` is the chronological byte length available for reads. Read offsets
are byte offsets from the oldest retained sample.

When enabled, the ACK trigger requests a freeze after a CRC-valid IN DATA packet
is accepted and ACKed by the host. The capture retains the preceding raw bus
history; the GUI can filter the decoded packet list to ACK-completed IN transactions.

The firmware rejects reads unless frozen and rejects ranges outside
`data_size`. This keeps host transfer from racing the capture writer.
