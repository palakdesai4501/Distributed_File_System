# Distributed File System using Socket Programming

## Overview

This project implements a distributed file system using socket programming in C. The system is composed of three servers (`Smain`, `Spdf`, and `Stext`) and supports multiple client connections. The main server (`Smain`) handles all client interactions, while the `Spdf` and `Stext` servers store files based on their types. The clients interact with `Smain` exclusively, and they are unaware of the internal distribution of files.

### Servers

- **Smain**: Main server that handles client requests and stores `.c` files locally.
- **Spdf**: Secondary server that stores `.pdf` files.
- **Stext**: Secondary server that stores `.txt` files.

### Client Operations

Clients can perform various operations by sending commands to `Smain`. The following commands are supported:

1. **ufile filename destination_path**: Uploads a file to the `Smain` server.
   - `.c` files are stored locally on `Smain`.
   - `.pdf` files are transferred to the `Spdf` server.
   - `.txt` files are transferred to the `Stext` server.

2. **dfile filename**: Downloads a file from the `Smain` server to the client's current working directory.
   - Retrieves `.c` files from `Smain`.
   - Retrieves `.pdf` files from `Spdf`.
   - Retrieves `.txt` files from `Stext`.

3. **rmfile filename**: Deletes a file from the `Smain` server.
   - Deletes `.c` files locally on `Smain`.
   - Requests `Stext` to delete `.txt` files.
   - Requests `Spdf` to delete `.pdf` files.

4. **dtar filetype**: Creates a tar file of all files of a specified type and downloads it to the client's current working directory.
   - For `.c` files, `Smain` creates the tar file.
   - For `.pdf` files, `Spdf` creates the tar file and sends it to `Smain`.
   - For `.txt` files, `Stext` creates the tar file and sends it to `Smain`.

5. **display pathname**: Lists all files in a specified directory path on `Smain`, combining files stored locally and remotely (`Spdf` and `Stext`).

## Project Structure

- `Smain.c`: Contains the code for the main server.
- `Spdf.c`: Contains the code for the server that handles `.pdf` files.
- `Stext.c`: Contains the code for the server that handles `.txt` files.
- `client24s.c`: Contains the client-side code to interact with `Smain`.

## How to Run the Application

1. **Compile the Code**:
   ```bash
   gcc -o Smain Smain.c
   gcc -o Spdf Spdf.c
   gcc -o Stext Stext.c
   gcc -o client24s client24s.c

2. **Start the Servers**:
   ```bash
   ./Smain
   ./Spdf
   ./Stext
   ./client24s

3. **Execute Commands**
   ```bash
   client24s$ ufile sample.c ~smain/folder1/folder2
   client24s$ dfile ~smain/folder1/folder2/sample.txt
   client24s$ rmfile ~smain/folder1/folder2/sample.pdf
   client24s$ dtar .txt
   client24s$ display ~smain/folder1


