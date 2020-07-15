# File Transfer Challenge

## Requirements

The solution has been developed and tested using:

  **Docker environment**
  - Docker version 19.03.8
  - docker-compose version 1.25.5

 - - -

## Building the project

Unzip (or clone the repository) locally. Within the project's root directory
execute:

```
   docker-compose build  
```

This will build the 2 docker images:
  - ft
  - ft_client_test

 - - -

## Running the project

### Launching the server
At the project's root directory, execute:
```
   docker-compose up ft
```

### Launching client tests

At the project's root directory, execute:
```
   docker-compose up --scale ft_client_test=N_INSTANCES ft_client_test 
```
Where, N_INSTACES are the number of ft_client_test container instances to launch.

### Running ft_client from command line

For running a client instance, use the following command:

```
   docker run -v ${pwd}:/files/:ro -it ft /ft_client [-d HOST] [-p PORT] [-u UUID] /files/FILE
```
Where:
  - HOST: IP address or domain where the ft_server container is being run
  - PORT: port in the host machine to which the ft_server is bound
  - UUID: client UUID
  - FILE: file to upload

> Note: in order to access the host's filesystem, the docker container must
> mount a host file or directory into the container's filesystem. In this case,
> by using `-v ${pwd}:/files/:ro`, the current directory in the host machine is
> being mount into the `/files` directory in the container's filesystem. This
> is why the FILE being uploaded must be prefixed with the `/files` path since
> this is where it will be located within the container's scope.

Example:
```
   docker run -v ${pwd}:/files/:ro -it ft /ft_client \
               -d 192.168.35.145 -p 4444 \
               -u e146b4c9-0ec0-4c6a-8bc7-5b9c90e37b1a \
               /files/LICENSE
```

 - - -

## Server operation

The _ft_server_ will place the received files in the `recv` folder. Within this
folder a sub-directory is created for each client instance named as its
respective _client UUID_. The uploaded files from each client instance are
stored separated on its client directory. This avoid collisions of files with
the same name being uploaded from different client instances.

For each file being uploaded the _ft_server_ creates a sibling metadata file.
Each metadata file holds:
 - the total file length (8 bytes);
 - the chunk length being used to transfer such a file (8 bytes);
 - BLAKE2 hash (64 bytes);
 - Progress bitmap (variable length): Each bit set to 1 represents a file chunk
   already received.

The metadata has 2 purposes:
 - Tracking the upload progress even after _ft_server_ process termination and
 re-start.
 - Support transfers of very large files without increasing the _ft_server_'s
 footprint in term of process's allocated memory.     

 - - -

## Protocol

The protocol has 4 type of messages:

 - **FILE_OFFER:** Sent from the client to the server to offer a file to upload.
 - **FILE_CHUNK_REQ:** Sent from the server to the client to request a file
 chunk.
 - **FILE_CHUNK_DATA:** Sent from the client to the server with the contents
 of a file chunk, in response to a _FILE_CHUNK_REQ_.
 - **FILE_COMPLETE:** Sent from the server to the client to notify the file is
 complete on the server side, in response to a _FILE_OFFER_ or a
 _FILE_CHUNK_DATA_.

### Message flow
 1. The client sends a _FILE_OFFER_ message to the server offering a file to
 upload;
 2. The server validates the file name and the file hash in the message. If it
 identifies that file is not complete, returns a _FILE_CHUNK_REQ_. If the file
 is complete, returns _FILE_COMPLETE_ finalizing the upload protocol;
 3. The client responds to the _FILE_CHUNK_REQ_ with a _FILE_CHUNK_DATA_ with
 the contents of the requested file chunk;
 4.  The server saves the contents of the  _FILE_CHUNK_DATA_ to the file and
 check if the file is complete. If the file is complete, returns _FILE_COMPLETE_
 finalizing the upload protocol. If not, sends a new _FILE_CHUNK_REQ_ requesting
 the next missing chunk.
 
   Steps 3 and 4 repeat until the file transfer is completed.

### Message Format

The message formats are detailed in the following tables. 

**Header**

All messages have the following header:

| Field Name   | Type(Size)      | Description                                          |
| ------------ | :-------------: | ---------------------------------------------------- |
| MAGIC        | Num (3)         | Fixed value: { 0x87, 0xFE, 0x77 }                    |
| message_type | Num (1)         | 1: OFFER, 2: CHUNK_REQ, 3: CHUNK_DATA, 4: COMPLETE   |
| message_len  | Num (2)         | Message remaining length                             |
| seq_number   | Num (2)         | Sequence number (incremented on each server request) |
| client_UUID  | Binary (16)     | Client identification                                |
| filename_len | Num (1)         | File name length                                     |
| filename     | Text (variable) | File name                                            |

**Offer fields**

| Field Name   | Type(Size)   | Description                                          |
| ------------ | :----------: | ---------------------------------------------------- |
| file_size    | Num (4)      | Total size of the file being transferred             |
| chunk_size   | Num (2)      | Size of the file chunks                              |
| file_hash    | Binary (64)  | BLAKE2 hash digest of the whole file contents        |

> Note: Number of chunks is calculated using the following algorithm:
> ```
>   n_chunks = file_size / chunk_size;
>   if ((file_size % chunk_size) > 0) n_chunks++;
> ```

**Chunk Request fields**
| Field Name   | Type(Size  ) | Description                                          |
| ------------ | :----------: | ---------------------------------------------------- |
| chunk_idx    | Num (4)      | Index of the file chunk being requested              |
| chunk_last   | Num (4)      | Not currently used                                   |

> Note: The file offset to the file chunk contents is calculated using:
> ```
>   offset = chunk_idx * chunk_size;
> ```

**Chunk Data fields**
| Field Name   | Type(Size)          | Description                                          |
| ------------ | :-----------------: | ---------------------------------------------------- |
| chunk_idx    | Num (4)             | Index of the file chunk being requested              |
| chunk_len    | Num (2)             | Length of the file chunk data                        |
| chunk_data   | Binary (variable)   | The file chunk data                                  |

> Note: The file offset to the file chunk contents is calculated using:
> ```
>   offset = chunk_idx * chunk_size;
> ```

**File Complete Fields**

This message has not additional fields.

### Know limitations

  - The maximum file length supported by the protocol and the implementations is
  4GB. This maximum length can be further limited by the underlying file system.

  - The current implementation uses chunk sizes of up to 3968 bytes so the
  message can be also fitted in UDP datagrams. By using larger chunk size with
  TCP connections would increase the protocol efficiency.

  - The protocol has been designed to allow the server requesting a range of
  chunks (by using the field `chunk_last`). Due to time constraints, these
  feature has not been implemented.

 - - -

## Architecture overview

The _ft_server_ and _ft_client_ share almost all the same code. The diagram
below depicts the major architectural components and how _ft_server_ and
_ft_client_ sit on top of them defining the expected behavior. 

![Figure 1 - components diagram](doc/fig1_components.svg?raw=true)

 - **file:** Responsible for reading and writing to files, chunking, calculating
 hashes, create, read and update metadata files;
 - **loop:** Provides support for the application main event loop. Includes
 socket polling and signal handling.
 - **netwrk:** Handles network sockets and connections.
 - **protocol:** Parsing, building and serialization of the protocol messages.
 - **request:** High level handling of requests.
 - **ft_client:** Instantiate, configures and links the underlying components
 and provides specific behavioral flows, according to the functional requirements
 for the client.
 - **ft_server:** Instantiate, configures and links the underlying components
 and provides specific behavioral flows, according to the functional requirements
 for the server.

 - - -

## Third party projects

This solution uses the following OSS projects:

  - [CryptoC++ v 8.2.0](https://github.com/weidai11/cryptopp/archive/CRYPTOPP_8_2_0.tar.gz)
  - boost: (boost-dev package from Alpine Linux)  
