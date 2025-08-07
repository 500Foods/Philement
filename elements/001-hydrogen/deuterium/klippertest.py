#!/usr/bin/env python3

import json
import asyncio
import os
import re
import logging
import psutil
import time

# Configure logging
logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')

def find_processes_and_config_files():
    processes = []
    config_files = []
    for proc in psutil.process_iter(['name', 'cmdline', 'pid']):
        cmdline = ' '.join(proc.info['cmdline'])
        if 'klippy.py' in cmdline:
            processes.append(('klipper', proc.info['pid'], proc))
            for arg in proc.info['cmdline']:
                if arg.endswith('.cfg'):
                    config_files.append(arg)
                    config_dir = os.path.dirname(arg)
                    moonraker_conf = os.path.join(config_dir, 'moonraker.conf')
                    mainsail_conf = os.path.join(config_dir, 'mainsail.conf')
                    if os.path.exists(moonraker_conf):
                        config_files.append(moonraker_conf)
                    if os.path.exists(mainsail_conf):
                        config_files.append(mainsail_conf)
        elif 'moonraker' in cmdline:
            processes.append(('moonraker', proc.info['pid'], proc))
            for arg in proc.info['cmdline']:
                if arg.startswith('-d'):
                    config_dir = arg[2:].strip()
                    moonraker_conf = os.path.join(config_dir, 'moonraker.conf')
                    if os.path.exists(moonraker_conf):
                        config_files.append(moonraker_conf)
        elif 'fluidd' in cmdline:
            processes.append(('fluidd', proc.info['pid'], proc))
        elif 'mainsail' in cmdline:
            processes.append(('mainsail', proc.info['pid'], proc))
        elif 'crowsnest' in cmdline:
            processes.append(('crowsnest', proc.info['pid'], proc))
            for arg in proc.info['cmdline']:
                if arg.endswith('.conf'):
                    config_files.append(arg)
        elif 'timelapse' in cmdline:
            processes.append(('timelapse', proc.info['pid'], proc))

    return processes, config_files

def parse_config_files(config_files):
    klipper_uds = None
    moonraker_port = None
    klipper_api_key = None
    moonraker_api_key = None
    config_data = {}

    for config_file in config_files:
        config_data[config_file] = {}
        with open(config_file, 'r') as f:
            content = f.read()

            key_value_pairs = re.findall(r'(\w+)\s*=\s*(.+?)\s*(?=\n|$)', content)
            for key, value in key_value_pairs:
                config_data[config_file][key] = value.strip()

            key_value_pairs = re.findall(r'(\w+)\s*:\s*(.+?)\s*(?=\n|$)', content)
            for key, value in key_value_pairs:
                config_data[config_file][key] = value.strip()

            json_style_pairs = re.findall(r'(\w+)\s*:\s*"(.+?)"\s*(?=\n|$)', content)
            for key, value in json_style_pairs:
                config_data[config_file][key] = value

    for config_file, data in config_data.items():
        if 'klippy_uds_address' in data:
            klipper_uds = data['klippy_uds_address']
            logging.debug(f"Found Klipper Unix Domain Socket address in {config_file}: {klipper_uds}")

        if 'unix_socket_path' in data:
            klipper_uds = data['unix_socket_path']
            logging.debug(f"Found Unix Domain Socket path in {config_file}: {klipper_uds}")
        elif 'socket_path' in data:
            klipper_uds = data['socket_path']
            logging.debug(f"Found Unix Domain Socket path in {config_file}: {klipper_uds}")

        if 'api_key' in data:
            if config_file.endswith('printer.cfg'):
                klipper_api_key = data['api_key']
                logging.debug(f"Found Klipper API key in {config_file}")
            elif config_file.endswith('moonraker.conf'):
                moonraker_api_key = data['api_key']
                logging.debug(f"Found Moonraker API key in {config_file}")

        if 'port' in data:
            moonraker_port = data['port']
            logging.debug(f"Found Moonraker port in {config_file}: {moonraker_port}")
        elif 'moonraker_port' in data:
            moonraker_port = data['moonraker_port']
            logging.debug(f"Found Moonraker port in {config_file}: {moonraker_port}")

        for key, value in data.items():
            if key.endswith('_config') and value.endswith('.conf'):
                if os.path.exists(value):
                    config_files.append(value)
                    logging.debug(f"Found additional config file in {config_file}: {value}")

    return klipper_uds, moonraker_port, klipper_api_key, moonraker_api_key

async def receive_response(reader):
    response_data = b''
    while True:
        try:
            chunk = await reader.read(4096)
            # logging.debug(f"Received chunk: {chunk}")
            if not chunk:
                break
            response_data += chunk
            # Check if the message is complete by looking for the framing character
            if b'\x03' in response_data:
                break
        except Exception as e:
            logging.error(f"Error while receiving response: {e}")
            break
    # Split the data on the framing character and take the first part as the complete message
    response_string = response_data.split(b'\x03')[0].decode()
    # logging.debug(f"Full response string: {response_string}")
    return response_string

async def send_request(writer, request):
    writer.write(request.encode())
    await writer.drain()
    # logging.debug(f"Sent request: {request}")


async def get_printer_status():
    start_time = time.time()
    processes, config_files = find_processes_and_config_files()
    logging.debug("Found processes:")
    for process_name, pid, process in processes:
        logging.debug(f"- {process_name} (PID: {pid}): {' '.join(process.info['cmdline'])}")

    logging.debug("\nFound config files:")
    for config_file in config_files:
        logging.debug(f"- {config_file}")

    klipper_uds, moonraker_port, klipper_api_key, moonraker_api_key = parse_config_files(config_files)

    if klipper_uds is None:
        logging.error("Error: Could not find Unix Domain Socket path.")
        return

    max_attempts = 3
    delay = 2

    for attempt in range(max_attempts):
        try:
            logging.debug(f"Attempting to connect to Unix Domain Socket: {klipper_uds} (Attempt {attempt + 1}/{max_attempts})")
            reader, writer = await asyncio.open_unix_connection(klipper_uds)
            logging.debug("Connected to Unix Domain Socket")

            list_request = json.dumps({
                "jsonrpc": "2.0",
                "method": "objects/list",
                "id": 123
            }) + "\x03"
            await send_request(writer, list_request)

            response = await receive_response(reader)
            logging.debug(f"Received response: {response}")
            if response:
                data = json.loads(response)
                logging.debug(f"Parsed response: {data}")

                if "result" in data:
                    logging.debug("API request succeeded")
                    objects = data["result"]["objects"]
                    print("Available objects:")
                    for obj in objects:
                        print(f"  - {obj}")

                    end_time = time.time()
                    print(f"Time taken to get initial response: {end_time - start_time:.6f} seconds")

                    # Subscribe to printer objects
                    subscribe_request = json.dumps({
                        "jsonrpc": "2.0",
                        "method": "objects/subscribe",
                        "params": {
                            "objects": {
                                "toolhead": ["position"]
                            }
                        },
                        "id": 456
                    }) + "\x03"

                    await send_request(writer, subscribe_request)
                    logging.debug("Sent JSON-RPC request to subscribe to printer objects")

                    while True:
                        try:
                            update = await receive_response(reader)
                            if update:
                                # logging.debug(f"Received update: {update}")
                                update_data = json.loads(update)
                                if "params" in update_data:
                                    # print("\nPrinter Status Update:")
                                    print_printer_status(update_data["params"]["status"])
                            else:
                                await asyncio.sleep(1)
                        except KeyboardInterrupt:
                            print("\nStopped listening for updates.")
                            break
                        except Exception as e:
                            logging.error(f"Error while receiving updates: {e}")
                            break
                else:
                    logging.error(f"Error in objects/list: {data.get('error', 'Unknown error')}")
            else:
                logging.error("No response received")

        except (json.JSONDecodeError, KeyError) as e:
            logging.error(f"Error: Unable to parse response: {e}")
        except Exception as e:
            logging.error(f"Error: {e}")

        await asyncio.sleep(delay)

    logging.error("Failed to establish communication with Klipper after multiple attempts.")

def print_printer_status(status):
    toolhead = status.get("toolhead", {})
    print(f"Toolhead Position: X={toolhead.get('position', [0, 0, 0])[0]:.2f}, "
          f"Y={toolhead.get('position', [0, 0, 0])[1]:.2f}, "
          f"Z={toolhead.get('position', [0, 0, 0])[2]:.2f}")

if __name__ == "__main__":
    asyncio.run(get_printer_status())
