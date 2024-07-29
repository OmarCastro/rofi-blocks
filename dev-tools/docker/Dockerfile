FROM ubuntu:latest
ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Africa/Johannesburg 

COPY entrypoints /entrypoints

RUN apt-get update \
    && apt-get install -y meson gcovr libjson-glib-1.0-0 libjson-glib-dev rofi rofi-dev valgrind xvfb libpng-dev libjpeg-dev libtiff-dev imagemagick sudo nodejs \
    && rm -rf /var/lib/apt/lists/* \
    && chmod +x /entrypoints/*

CMD /entrypoints/run-ui-tests.entrypoint.sh
