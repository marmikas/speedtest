# Speedtest CLI

A command-line internet speed testing utility written in C using **libcurl** and **cJSON**.

The program can determine the user's location, select a suitable Speedtest server, and measure download and upload speeds.

## Features

* Download speed test
* Upload speed test
* Automatic location detection
* Best server selection based on latency
* Manual server selection by ID
* Full automatic speed test
* Command-line interface

## Project Structure

```text
├── src/        # Source files
├── include/    # Project headers
├── lib/        # cJSON
├── data/       # Speedtest server list
└── Makefile
```

## Dependencies

* GCC or Clang
* libcurl
* Make

cJSON is included directly in the project.

### macOS

Install libcurl using Homebrew:

```bash
brew install curl
```

Build:

```bash
make
```

### Linux

On Debian/Ubuntu:

```bash
sudo apt update
sudo apt install build-essential libcurl4-openssl-dev
```

Then build:

```bash
make
```

On Fedora:

```bash
sudo dnf install gcc make libcurl-devel
make
```

## Usage

```bash
./speedtest [options]
```

Options:

```text
-d          Download test
-u          Upload test
-l          Show location
-b          Find best server
-s ID       Select server by ID
-a          Run full speed test
-h          Show help
```

For example:

```bash
./speedtest -a
```

Or test download speed using a specific server:

```bash
./speedtest -d -s 16249
```

Clean build files with:

```bash
make clean
```

## Technologies

C, libcurl, cJSON, Make
