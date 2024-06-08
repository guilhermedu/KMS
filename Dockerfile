FROM ubuntu:22.04

WORKDIR /KMS

RUN apt-get update && apt-get install -y \
    build-essential \
    gcc-11 g++-11   \
    uuid-dev        \
    libmysqlcppconn-dev \
    mysql-client && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 60 --slave /usr/bin/g++ g++ /usr/bin/g++-11

COPY ./eigen ./eigen
COPY ./include ./include
COPY ./lib ./lib
COPY ./KMS ./KMS


WORKDIR /KMS/KMS

RUN make