# sys-cricket
sys-cricket is a sysmodule for the Nintendo Switch that allows for streaming values in memory over UDP to listening clients. This is useful for debugging, reverse engineering, speedrunnig, and other applications. It listens on port 42424 by default. Send a UDP message containing your destination port and it will immediately begin streaming UDP data to that port. It sends it to the IP address that sent the connection message.

## ACTIVE DEVELOPMENT
This tool is in active development. It was originally started as a side project to develop a snapshot tool for Tears of the Kingdom but while exploring how this works, a more general approach was adopted. As the speedrunning community became more interested in blindfolded runs, it was decided that simply reading out precise information (like the player's and camera's full transformation as well as velocities and relative distances) would be immediately useful. This is the current focus of development.

Currently, it will only read specific locations defined at compile time (the transformation matrices for the player character and camera) but it is planned to allow custom locations with optional pointer offsets to be specified when connecting as well as the shape, size, and format of the data expected there.

## Planned Features
There is a vague intention to implement the following:
- Allowing clients to specify the specific parts of memory to be read (with pointer offsets)
- Custom polling intervals for each requested memory location
- Starting and stopping of data streaming

## Installing
1. Download the zip from the latest releases
2. Copy the zip file contents to your switch SD card at `/atmosphere/contents`

## Developer Instructions
The project is fundamentally built around devkitpro (like most switch homebrew). A Dockerfile is included that sets up a cross-compiling toolchain in a linux container. Use it as you see fit!

One easy way is to make sure you have Docker installed and then use VS Code with the Remote - Containers extension. Open the folder in a container and tell it to use the existing Dockerfile. It will set up the environment for you and you can build and run the project from there.

The following commands are useful for building and deploying or debugging the project:
- `make` - Builds the project
- `make clean` - Cleans previous intermediate files
- `make deploy-ftp` - Builds the project and sends it to the switch via ftp

In order for ftp deployment to work, you need an ftp server installed and running on your switch (like [sys-ftps-light](https://github.com/cathery/sys-ftpd)). You can define your ftp credentials a .env file (see example.env for details).

There are some useful utilities in the 'util' folder:
- `util\nxlink.js` - A node.js script that listens for log messages and prints them to the console.
- `util\client.js` - An example client written in node.js that echos messages from sys-cricket.

The node.js tools are best run on the local computer and not in the docker container and do require using npm (or yarn) to install dependencies first. Scripts are defined in the package.json. You can also define connection settings in util/.env and there is an example here too.

## Customizing the Dev Environment
Many scripts and tools need to know the IP address of the switch as well as ftp credentials as you've defined them on your switch. These are defined in the .env file in the root directory and in the util directory. See example.env for the expected format and supported variables.

## Client Applications
Any client application can communicate with sys-cricket via UDP.  An example client application (written in node.js) is available at [client-cricket](https://github.com/Olliebrown/client-cricket).  Messages are always sent and received in JSON and documentation will be provided for the expected structure of these messages in the future.

## Credits
I explored and emulated a multitude of other switch homebrew projects while working on this including:
- sys-http
- botw-save-state
- Many examples in the libnx repository
