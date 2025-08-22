FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    libmicrohttpd-dev \
    libcurl4-openssl-dev \
    openssl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -days 365 -nodes -subj "/CN=localhost"

RUN gcc gateway.c -lmicrohttpd -lcurl -o gateway

CMD ["./gateway"]