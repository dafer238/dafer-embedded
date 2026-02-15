## Setting the Port
# Before running the commands, set the AMPY_PORT environment variable to COM4. This avoids needing to specify the port in each command.

```bash
export AMPY_PORT=COM4
```


## Listing Files
# List all files on the device

```bash
ampy ls
```



## Uploading a File
# Upload a file named main.py from your local machine to the device

```bash
ampy put main.py
```



## Downloading a File
# Download a file named boot.py from the device to your local machine

```bash
ampy get boot.py
```



## Running a File
# Run a Python script directly on the device

```bash
ampy run main.py
```



## Deleting a File
# Delete a file named main.py from the device

```bash
ampy rm main.py
```



## Making a Directory
# Create a new directory on the device

```bash
ampy mkdir /new_directory
```



## Removing a Directory
# Remove a directory from the device

```bash
ampy rmdir /new_directory
```



## Reading the REPL
# Connect to the device's REPL

```bash
ampy --port COM4 repl
```