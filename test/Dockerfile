################################################################################
###  Test client docker image
#
# The purpose of this docker image is to run a client that uploads all the
# files in the /files directory. 
#
# On each instace of this container, the client_uuid is automatically generated.
# On the other hand, all invocation of ft_client within the same instance will
# use the same client_uuid.
#
###  # Released under MIT License
###  Copyright (c) 2020 Hernan Perrone (hernan.perrone@gmail.com)
################################################################################

# Base the test image on the server/client image
FROM ft:latest

# Copy all the test files to be transfered to the image
COPY ./files /files

COPY ./ft_client_test.sh /ft_client_test.sh
RUN chmod a+x /ft_client_test.sh

CMD ["/ft_client_test.sh"]