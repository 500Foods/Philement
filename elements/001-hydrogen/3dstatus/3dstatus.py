import json
import socket
import psutil
import os
import re
import logging
import time
import jsonschema

logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')

def find_processes_and_config_files():
    processes = []
    config_files = []
    for proc in psutil.process_iter(['name', 'cmdline']):
        cmdline = ' '.join(proc.info['cmdline'])
        if 'klippy.py' in cmdline:
            processes.append(('klipper', proc))
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
            processes.append(('moonraker', proc))
            for arg in proc.info['cmdline']:
                if arg.startswith('-d'):
                    config_dir = arg[2:].strip()
                    moonraker_conf = os.path.join(config_dir, 'moonraker.conf')
                    if os.path.exists(moonraker_conf):
                        config_files.append(moonraker_conf)
        elif 'fluidd' in cmdline:
            processes.append(('fluidd', proc))
        elif 'mainsail' in cmdline:
            processes.append(('mainsail', proc))
        elif 'crowsnest' in cmdline:
            processes.append(('crowsnest', proc))
            for arg in proc.info['cmdline']:
                if arg.endswith('.conf'):
                    config_files.append(arg)
        elif 'timelapse' in cmdline:
            processes.append(('timelapse', proc))

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

            # Handle key=value format
            key_value_pairs = re.findall(r'(\w+)\s*=\s*(.+?)\s*(?=\n|$)', content)
            for key, value in key_value_pairs:
                config_data[config_file][key] = value.strip()

            # Handle key: value format
            key_value_pairs = re.findall(r'(\w+)\s*:\s*(.+?)\s*(?=\n|$)', content)
            for key, value in key_value_pairs:
                config_data[config_file][key] = value.strip()

            # Handle JSON-style key: "value" format
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

def check_systemd_service(service_name):
    service_config = os.path.join('/etc/systemd/system', f'{service_name}.service')
    if os.path.exists(service_config):
        with open(service_config, 'r') as f:
            service_content = f.read()
            user_match = re.search(r'User=(\w+)', service_content)
            if user_match:
                service_user = user_match.group(1)
                logging.debug(f"{service_name} runs under user: {service_user}")


def receive_response(sock):
    response_data = b''
    while True:
        try:
            chunk = sock.recv(4096)
            if not chunk:
                break
            response_data += chunk
        except socket.error:
            break
    response_string = response_data.decode().strip('\x03')  # Strip any framing characters
    return response_string
def get_printer_status():
    processes, config_files = find_processes_and_config_files()
    logging.debug("Found processes:")
    for process_name, process in processes:
        logging.debug(f"- {process_name}: {' '.join(process.info['cmdline'])}")

    logging.debug("\nFound config files:")
    for config_file in config_files:
        logging.debug(f"- {config_file}")

    klipper_uds, moonraker_port, klipper_api_key, moonraker_api_key = parse_config_files(config_files)

    if klipper_uds is None:
        logging.error("Error: Could not find Unix Domain Socket path.")
        return

    logging.debug("\nChecking systemd service configurations:")
    for process_name, _ in processes:
        check_systemd_service(process_name)

    max_attempts = 3
    delay = 2  # Delay in seconds between attempts

    for attempt in range(max_attempts):
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
            try:
                logging.debug(f"Attempting to connect to Unix Domain Socket: {klipper_uds} (Attempt {attempt + 1}/{max_attempts})")
                sock.setblocking(0)  # Set non-blocking mode
                try:
                    sock.connect(klipper_uds)
                except socket.error as e:
                    if e.errno == errno.ECONNREFUSED:
                        time.sleep(0.1)
                        continue
                    raise
                sock.setblocking(1)  # Set blocking mode after successful connection
                logging.debug("Connected to Unix Domain Socket")

                sock.settimeout(10)  # Set a timeout of 10 seconds for receiving the response

                # Send the "objects/list" request
                list_request = json.dumps({
                    "jsonrpc": "2.0",
                    "method": "objects/list",
                    "id": 123
                }) + "\x03"  # Adding framing character
                sock.sendall(list_request.encode())
                logging.debug("Sent JSON-RPC request for objects/list")

                try:
                    response = receive_response(sock)
                    logging.debug(f"Received response: {response}")
                    data = json.loads(response)

                    if "result" in data:
                        logging.debug("API request succeeded")
                        objects = data["result"]["objects"]
                        print(f"Available objects: {objects}")

                        # Construct the main API request for toolhead and print_stats
                        if klipper_api_key:
                            request = json.dumps({
                                "jsonrpc": "2.0",
                                "method": "printer.objects.query",
                                "params": {
                                    "objects": {
                                        "toolhead": ["position"],
                                        "print_stats": ["state"]
                                    }
                                },
                                "id": 1,
                                "api_key": klipper_api_key
                            }) + "\x03"  # Adding framing character
                        else:
                            request = json.dumps({
                                "jsonrpc": "2.0",
                                "method": "printer.objects.query",
                                "params": {
                                    "objects": {
                                        "toolhead": ["position"],
                                        "print_stats": ["state"]
                                    }
                                },
                                "id": 1
                            }) + "\x03"  # Adding framing character

                        sock.sendall(request.encode())
                        logging.debug("Sent JSON-RPC request for toolhead and print_stats")

                        response = receive_response(sock)
                        logging.debug(f"Received response: {response}")
                        data = json.loads(response)

                        if "result" in data:
                            toolhead_position = data["result"]["status"]["toolhead"]["position"]
                            print_state = data["result"]["status"]["print_stats"]["state"]

                            print(f"Toolhead Position: X={toolhead_position[0]}, Y={toolhead_position[1]}, Z={toolhead_position[2]}")
                            print(f"Print State: {print_state}")
                            return  # Exit the function after successful communication
                        else:
                            logging.error("Error: Failed to retrieve printer status.")

                    else:
                        logging.warning("API request failed. Retrying...")

                except (json.JSONDecodeError, KeyError) as e:
                    logging.error(f"Error: Unable to parse response: {e}")

            except socket.timeout:
                logging.warning("Timed out while waiting for response. Retrying...")
            except Exception as e:
                logging.error(f"Error: {e}")

        time.sleep(delay)  # Wait for a short delay before the next attempt

    logging.error("Failed to establish communication with Klipper after multiple attempts.")

if __name__ == "__main__":
    get_printer_status()
