###############################################################################
# Build environment
###############################################################################
FROM alpine:3.11 AS builder

RUN apk --no-cache add \
    cmake make gcc musl-dev bison flex openssl-dev zlib-dev \
 && adduser -D build

COPY --chown=build:build . /home/build/Pancake2

USER build:build
# Enable all modules except for the OpenSSL module which needs an unsupported
# OpenSSL version. We still need openssl-dev for the Authentication module.
# Also disable unused Select since we can use LinuxPoll
RUN mkdir /home/build/Pancake2/build \
 && cd /home/build/Pancake2/build \
 && cmake -DPANCAKE_AUTHENTICATION=ON -DPANCAKE_AUTHENTICATION_FILE=ON \
    -DPANCAKE_HTTP_BASIC_AUTHENTICATION=ON -DPANCAKE_HTTP_DEFLATE=ON \
    -DPANCAKE_HTTP_FASTCGI=ON -DPANCAKE_HTTP_REWRITE=ON \
    -DPANCAKE_LINUX_POLL=ON -DPANCAKE_CONFIG_PATH=/etc/Pancake2/pancake.cfg \
    .. \
 && make

###############################################################################
# Runtime environment
###############################################################################
FROM alpine:3.11

RUN apk --no-cache add \
    libssl1.1 zlib

RUN addgroup -S www-data \
 && adduser -S -G www-data -H -h /var/www -D www-data

COPY --from=builder /home/build/Pancake2/build/Pancake /usr/bin/
COPY config/default.cfg /etc/Pancake2/pancake.cfg
COPY docs/agile-development-process.jpg /var/www/index.jpg

EXPOSE 8080

USER www-data:www-data
CMD ["Pancake"]
