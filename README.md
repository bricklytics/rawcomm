# rawcomm

A modular C++ networking project for experimenting with custom protocol stacks, raw sockets, and layered communication models. Designed for educational and research purposes, it features a clear separation of concerns across data, error control, flow control, protocol, and presentation layers.

## Features

- **Layered Architecture:** Each network layer is implemented as a separate module for easy extensibility and testing.
- **Raw Socket Communication:** Direct use of raw sockets for low-level network experimentation.
- **Custom Protocols:** Includes a Kermit-like protocol implementation.
- **Error and Flow Control:** Pluggable strategies for error detection (e.g., checksum) and flow control (e.g., Stop-and-Wait).
- **Client & Server UIs:** Ncurses-based user interfaces for both client and server.
- **Test Suite:** Modular tests for protocol and layer components.

## Project Structure

```
rawcomm/
├── app/                  # Application entry points and main logic
├── datalayer/            # Data transfer and socket setup
├── errorcontrollayer/    # Error control strategies (e.g., checksum)
├── flowcontrollayer/     # Flow control strategies (e.g., Stop-and-Wait)
├── presentationlayer/    # UI and logging utilities
├── protocollayer/        # Protocol implementations (e.g., Kermit)
├── objetos/              # Sample files for transfer
├── tesouros/             # Additional sample files
├── build/                # Build output directory
├── Makefile              # Build system
```

## Building

Requirements:
- `clang++-19` (or compatible C++17 compiler)
- `make`
- `ncurses` and `pthread` libraries

To build both client and server:
```sh
make target_server
make target_client
```
Binaries will be placed in the `build/` directory.

To clean build artifacts:
```sh
make clean
```

## Running

Run the server or client (may require `sudo` for raw sockets):

```sh
sudo ./build/server <interface> <optional logfile>
sudo ./build/client <interface> <optional logfile>
```

When not providing logfile output will be sent do "/dev/null"

## Customization

- Add new protocols or strategies by extending the appropriate `feature` directories.
- Place test files in `objetos/` or `tesouros/` for transfer experiments.

## License

This project is for educational and research use. <br> 
See LICENSE for details.

