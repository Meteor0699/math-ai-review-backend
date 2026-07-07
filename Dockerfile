# 多阶段构建：编译阶段
FROM ubuntu:22.04 AS builder

# 避免交互式提示
ENV DEBIAN_FRONTEND=noninteractive

# 安装编译依赖
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

# 编译安装 Drogon (使用稳定版本 v1.9.0)
RUN git clone --branch v1.9.0 --depth 1 https://github.com/drogonframework/drogon.git /tmp/drogon && \
    cd /tmp/drogon && \
    git submodule update --init --recursive && \
    cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_ORM=ON -DBUILD_MYSQL=ON && \
    cmake --build build -j$(nproc) && \
    cmake --install build && \
    rm -rf /tmp/drogon

# 设置工作目录
WORKDIR /build

# 复制源代码
COPY . .

# 编译
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build -j$(nproc)

# 运行阶段
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# 安装运行时依赖
RUN apt-get update && apt-get install -y \
    libmariadb-dev \
    libssl-dev \
    libjsoncpp-dev \
    uuid-dev \
    zlib1g-dev \
    libcurl4 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# 复制编译好的二进制文件
COPY --from=builder /build/build/math-ai-review-backend /app/
COPY --from=builder /build/config.json /app/

WORKDIR /app

# 创建上传目录
RUN mkdir -p uploads/papers

# 暴露端口
EXPOSE 8080

# 启动命令
CMD ["./math-ai-review-backend"]
