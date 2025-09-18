FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libmicrohttpd-dev \
    libcurl4-openssl-dev \
    libcjson-dev \
    libmongoc-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN git clone --recursive https://github.com/GlitchedPolygons/l8w8jwt.git tmp && \
    cd tmp && mkdir -p build && cd build && \
    cmake -DBUILD_SHARED_LIBS=Off -DL8W8JWT_PACKAGE=On -DCMAKE_BUILD_TYPE=Release .. &&\
    cmake --build . --config Release && \
    cd ../.. && \
    mv ./tmp/build ./l8w8jwt && \
    rm -rf ./tmp && \
    gcc model.c repo.c main.c \
    -Il8w8jwt/l8w8jwt/include/l8w8jwt \
    -Wl,-Bstatic \
        l8w8jwt/l8w8jwt/bin/release/libl8w8jwt.a \
        l8w8jwt/mbedtls/library/libmbedtls.a \
        l8w8jwt/mbedtls/library/libmbedx509.a \
        l8w8jwt/mbedtls/library/libmbedcrypto.a \
    -Wl,-Bdynamic \
        -lcjson -lcurl -lmicrohttpd $(pkg-config --cflags --libs libmongoc-1.0) \
    -o project_service

CMD ["./project_service"]
