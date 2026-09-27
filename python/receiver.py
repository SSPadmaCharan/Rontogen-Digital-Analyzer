import serial
import time
from waveform import WaveformViewer


# -----------------------------------------
# CONFIGURATION
# -----------------------------------------

PORT = "COM3"
BAUD_RATE = 115200

MAGIC_1 = 0xAA
MAGIC_2 = 0x55

PACKET_SIZE = 512
SAMPLE_COUNT = 500


# -----------------------------------------
# CRC-16-CCITT
# -----------------------------------------

def crc_update(crc, data):
    crc ^= data << 8

    for _ in range(8):

        if crc & 0x8000:
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF

        else:
            crc = (crc << 1) & 0xFFFF

    return crc


def calculate_crc(data):

    crc = 0xFFFF

    for byte in data:
        crc = crc_update(crc, byte)

    return crc


# -----------------------------------------
# OPEN SERIAL PORT
# -----------------------------------------

print("Connecting to Rontogen Analyzer...")
viewer = WaveformViewer()

ser = serial.Serial(
    PORT,
    BAUD_RATE,
    timeout=1
)

time.sleep(2)

print("Connected.")
print("Waiting for packets...")


# -----------------------------------------
# MAIN RECEIVER LOOP
# -----------------------------------------

while True:

    byte = ser.read(1)

    if not byte:
        continue

    value = byte[0]

    # Look for first magic byte
    if value != MAGIC_1:
        continue

    # Look for second magic byte
    byte = ser.read(1)

    if not byte:
        continue

    if byte[0] != MAGIC_2:
        continue

    print("\nPacket detected!")


    # -------------------------------------
    # READ REMAINING PACKET
    # -------------------------------------

    remaining = ser.read(PACKET_SIZE - 2)

    if len(remaining) != PACKET_SIZE - 2:

        print("ERROR: Incomplete packet")
        continue


    # Complete packet
    packet = bytes([MAGIC_1, MAGIC_2]) + remaining


    # -------------------------------------
    # DECODE HEADER
    # -------------------------------------

    version = packet[2]

    packet_type = packet[3]


    # Little-endian 16-bit value
    sample_count = (
        packet[4]
        | (packet[5] << 8)
    )


    # Little-endian 32-bit value
    sample_rate = (
        packet[6]
        | (packet[7] << 8)
        | (packet[8] << 16)
        | (packet[9] << 24)
    )


    # -------------------------------------
    # EXTRACT PAYLOAD
    # -------------------------------------

    samples = packet[10:510]
    ch0 = [(sample >> 0) & 1 for sample in samples]
    ch1 = [(sample >> 1) & 1 for sample in samples]
    ch2 = [(sample >> 2) & 1 for sample in samples]
    ch3 = [(sample >> 3) & 1 for sample in samples]

    print("\nFirst 20 channel values:")

    print("CH0:", ch0[:20])
    print("CH1:", ch1[:20])
    print("CH2:", ch2[:20])
    print("CH3:", ch3[:20])


    # -------------------------------------
    # RECEIVED CRC
    # -------------------------------------

    received_crc = (
        packet[510]
        | (packet[511] << 8)
    )


    # -------------------------------------
    # CALCULATE CRC
    # -------------------------------------

    calculated_crc = calculate_crc(
        packet[2:510]
    )


    # -------------------------------------
    # DISPLAY PACKET INFORMATION
    # -------------------------------------

    print("--------------------------------")
    print("Rontogen Capture")
    print("--------------------------------")

    print(f"Version       : {version}")
    print(f"Packet type   : {packet_type}")
    print(f"Sample count  : {sample_count}")
    print(f"Sample rate   : {sample_rate} Hz")
    print(f"Packet size   : {len(packet)} bytes")

    print(f"Received CRC  : 0x{received_crc:04X}")
    print(f"Calculated CRC: 0x{calculated_crc:04X}")


    # -------------------------------------
    # CHECK CRC
    # -------------------------------------
    if received_crc == calculated_crc:
     print("CRC STATUS    : OK")
    viewer.update(samples, sample_rate)
else:
    print("CRC STATUS    : ERROR")


    # -------------------------------------
    # SHOW FIRST SAMPLES
    # -------------------------------------

    print("\nFirst 20 samples:")

    print(samples[:20])