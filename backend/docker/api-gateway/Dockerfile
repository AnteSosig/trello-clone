FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    libmicrohttpd-dev \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN gcc gateway.c -lmicrohttpd -lcurl -o gateway

CMD ["./gateway"]