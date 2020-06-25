################################################################################
###  File Transfer image - client and server
#
# This docker image contains the compiled ft_server and ft_client.
#
# It is based on alpine linux and it is multistaged built: fisrt build the
# binaries and then copy the built binaries to a "production" image.
#
# When launched, the container runs the ft_server by default.
#
# Mount points:
#  - /in: a folder where the ft_server writes the received files
#
###  # Released under MIT License
###  Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
################################################################################

# This is a multistaged image. First use a Linux alpine image with g++ and cmake
# to build binaries.
FROM alpine:3.12 AS GCC_BUILD

# Include g++ and cmake
RUN apk upgrade -U \
    && apk add --no-cache ca-certificates build-base g++ cmake boost-dev

# Download and build libcrypto++
WORKDIR /tmp
RUN wget https://github.com/weidai11/cryptopp/archive/CRYPTOPP_8_2_0.tar.gz
RUN tar xfz CRYPTOPP_8_2_0.tar.gz
WORKDIR /tmp/cryptopp-CRYPTOPP_8_2_0
RUN make

WORKDIR /tmp
# Copy the project files to the build image
COPY ./CMakeLists.txt /tmp/CMakeLists.txt
COPY ./src /tmp/src


# Build the binaries
RUN mkdir /tmp/build
WORKDIR /tmp/build
RUN cmake /tmp/
RUN make ft_client ft_server;

# Then, create the actual target image based on Linux alpine and copy the
# application binaries.
FROM alpine:3.12

RUN apk upgrade -U \
    && apk add --no-cache ca-certificates libstdc++

COPY --from=GCC_BUILD /tmp/build/ft_client /ft_client
COPY --from=GCC_BUILD /tmp/build/ft_server /ft_server

# set the default command to launch the ft_server
CMD ["/ft_server"]
