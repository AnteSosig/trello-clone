FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libmicrohttpd-dev \
    libcjson-dev \
    uuid-dev \
    pkg-config \
    libuv1-dev \
    libssl-dev \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/* \
    && git clone --depth 1 --branch 2.16.2 https://github.com/datastax/cpp-driver.git /tmp/cpp-driver \
    && cd /tmp/cpp-driver \
    && mkdir build \
    && cd build \
    && cmake -DCASS_BUILD_STATIC=ON -DCASS_BUILD_SHARED=ON -DCMAKE_BUILD_TYPE=Release .. \
    && make -j2 \
    && make install \
    && ldconfig \
    && cp /usr/local/lib/x86_64-linux-gnu/libcassandra_static.a /usr/local/lib/ \
    && cd / \
    && rm -rf /tmp/cpp-driver

WORKDIR /app

COPY . .

RUN git clone --recursive https://github.com/GlitchedPolygons/l8w8jwt.git tmp && \
    cd tmp && mkdir -p build && cd build && \
    cmake -DBUILD_SHARED_LIBS=Off -DL8W8JWT_PACKAGE=On -DCMAKE_BUILD_TYPE=Release .. &&\
    cmake --build . --config Release && \
    cd ../.. && \
    mv ./tmp/build ./l8w8jwt && \
    rm -rf ./tmp && \
    gcc -c model.c repo.c jwt_middleware.c main.c \
        -Il8w8jwt/l8w8jwt/include/l8w8jwt \
        -I/usr/local/include && \
    g++ -o notification_service *.o \
        -Wl,-Bstatic \
            l8w8jwt/l8w8jwt/bin/release/libl8w8jwt.a \
            l8w8jwt/mbedtls/library/libmbedtls.a \
            l8w8jwt/mbedtls/library/libmbedx509.a \
            l8w8jwt/mbedtls/library/libmbedcrypto.a \
            /usr/local/lib/libcassandra_static.a \
        -Wl,-Bdynamic \
            -lcjson -lmicrohttpd -luuid -luv -lssl -lcrypto -lz -lstdc++ -lpthread

CMD ["./notification_service"]
