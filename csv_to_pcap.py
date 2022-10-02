import argparse
import csv
import struct

def main():
	"""Entry point"""

	# Parse arguments
	parser = argparse.ArgumentParser()
	parser.add_argument("input", help="Input filename")
	parser.add_argument("output", help="Output filename")
	args = parser.parse_args()

	# Open input and output files
	input = open(args.input, "r")
	output = open(args.output, "wb")

	# Write file header
	output.write(generate_file_header())

	# Create CSV reader
	input_reader = csv.reader(input)

	# Skip header row
	next(input_reader)

	# Iterate over rows extracting and outputting packets
	for row in input_reader:
		# Split row into fields
		row_time = row[0]
		row_crc_valid = row[1]
		row_data = row[2:]

		# Convert strings to appropriate types
		row_time = float(row_time)
		row_crc_valid = (row_crc_valid == "true")
		row_data = bytearray([int(x, base=16) for x in row_data])

		# Write valid packets
		if row_crc_valid:
			output.write(generate_record_header(row_time, len(row_data)))
			output.write(row_data)

	# Close files
	input.close()
	output.close()

def generate_file_header():
	"""Generate PCAP file header for LocalTalk"""

	return struct.pack("@IHHiIII",
					   0xa1b2c3d4, # Magic number
					   2, # Major version number
					   4, # Minor version number
					   0, # GMT to local timezone correction (always zero)
					   0, # Accuracy of timestamps (all tools set to zero)
					   65535, # Max size of packets (fixed large value)
					   114 # Link type LOCALTALK from https://www.tcpdump.org/linktypes.html
					  )

def generate_record_header(timestamp, length):
	"""Generate PCAP file record header"""

	return struct.pack("@IIII",
					   int(timestamp), # Timestamp seconds
					   int((timestamp - int(timestamp)) * 1000000), # Timestamp microseconds
					   length, # Packet data length in file (octets)
					   length, # Actual length of packet (octets)
					  )

if __name__ == "__main__":
	main()
