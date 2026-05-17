#!/bin/bash
# CloudDisk 一键启动脚本
# 用法: ./start.sh [--docker | --native | --stop]

set -e

MODE="${1:---docker}"

start_docker() {
    echo "🚀 启动基础设施 (MySQL + Consul + RabbitMQ)..."
    docker compose up -d
    echo "⏳ 等待 MySQL 就绪..."
    until docker exec clouddisk-mysql mysqladmin ping -h localhost --silent 2>/dev/null; do
        sleep 2
    done
    echo "✅ MySQL 就绪"
    echo "✅ 基础设施已启动"
}

start_services() {
    echo "🚀 启动 UserService..."
    if [ ! -f ./UserService ]; then
        echo "⚠️  UserService 未编译，跳过"
    else
        ./UserService &>/tmp/clouddisk-user.log &
        echo "✅ UserService 已启动 (PID: $!)"
    fi

    echo "🚀 启动 Server..."
    if [ ! -f ./server ]; then
        echo "⚠️  server 未编译，跳过"
    else
        ./server &>/tmp/clouddisk-server.log &
        echo "✅ Server 已启动 (PID: $!)"
    fi
}

stop_all() {
    echo "🛑 停止服务..."
    pkill -f "^./server" 2>/dev/null || true
    pkill -f "^./UserService" 2>/dev/null || true
    docker compose down 2>/dev/null || true
    echo "✅ 已停止"
}

case "$MODE" in
    --docker)
        start_docker
        start_services
        echo ""
        echo "🌐 Web:    http://localhost:8888/web"
        echo "📁 Files:  http://localhost:8888"
        echo "🔗 Consul: http://localhost:8500"
        echo "🐰 MQ:     http://localhost:15672 (guest/guest)"
        echo ""
        echo "📋 查看日志:"
        echo "  tail -f /tmp/clouddisk-server.log"
        echo "  tail -f /tmp/clouddisk-user.log"
        ;;
    --native)
        echo "使用本地服务（MySQL/Consul/RabbitMQ 请在外部启动）"
        start_services
        ;;
    --stop)
        stop_all
        ;;
    *)
        echo "用法: ./start.sh [--docker | --native | --stop]"
        ;;
esac
