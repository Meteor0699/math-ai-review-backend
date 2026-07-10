FROM node:20-bookworm-slim AS frontend-builder

WORKDIR /frontend
COPY frontend/package*.json ./
RUN npm install
COPY frontend/ ./
RUN npm run build

FROM ubuntu:22.04 AS backend-builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libmariadb-dev \
    libssl-dev \
    libjsoncpp-dev \
    uuid-dev \
    zlib1g-dev \
    libcurl4-openssl-dev \
    wget \
    && rm -rf /var/lib/apt/lists/*

ARG DROGON_VERSION=v1.9.13
RUN git clone --branch ${DROGON_VERSION} --depth 1 https://github.com/drogonframework/drogon.git /tmp/drogon && \
    cd /tmp/drogon && \
    git submodule update --init --recursive && \
    cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_ORM=ON -DBUILD_MYSQL=ON && \
    cmake --build build -j$(nproc) && \
    cmake --install build && \
    rm -rf /tmp/drogon

WORKDIR /build
COPY . .

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build -j$(nproc) && \
    ctest --test-dir build --output-on-failure

FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    libmariadb-dev \
    libssl-dev \
    libjsoncpp-dev \
    uuid-dev \
    zlib1g-dev \
    libcurl4 \
    mariadb-client \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY --from=backend-builder /build/build/math-ai-review-backend /app/
COPY --from=backend-builder /build/config.json /app/
COPY --from=backend-builder /build/ca.pem /app/
COPY --from=backend-builder /build/database /app/database/
COPY --from=backend-builder /build/scripts/migrate-db.sh /app/scripts/migrate-db.sh
COPY --from=backend-builder /build/scripts/docker-entrypoint.sh /app/scripts/docker-entrypoint.sh
COPY --from=frontend-builder /frontend/dist /app/frontend_dist/

WORKDIR /app

RUN mkdir -p uploads/papers && chmod +x /app/scripts/migrate-db.sh /app/scripts/docker-entrypoint.sh

EXPOSE 8080

CMD ["/app/scripts/docker-entrypoint.sh"]
