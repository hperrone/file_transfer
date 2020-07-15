#!/bin/sh

################################################################################
###  # Released under MIT License
###  Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
################################################################################

FILES=$(find /files -type f)

# Hostname as defined in docker compose script
HOSTNAME=ft

## Iterate over all files and send them to the server
for f in $FILES; do
    /ft_client -d ${HOSTNAME} ${f};
done;
