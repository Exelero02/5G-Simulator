FROM ubuntu:latest

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    g++ \
    make \
    git \
    libboost-all-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY CMakeLists.txt /app/
COPY main.cpp /app/

RUN mkdir /app/build/ && cd /app/build/ && cmake .. && make

CMD ["/app/build/5GSim"]
