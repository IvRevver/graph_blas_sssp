set -e  # Выход при ошибке
set -u  # Выход при использовании неопределённой переменной

GRAPHBLAS_REPO="https://github.com/DrTimothyAldenDavis/GraphBLAS.git"
LAGRAPH_REPO="https://github.com/GraphBLAS/LAGraph.git"
GRAPHBLAS_DIR="/tmp/GraphBLAS"
LAGRAPH_DIR="/tmp/LAGraph"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

PKG_MANAGER=""

print_message() {
    local color=$1
    local message=$2
    echo -e "${color}${message}${NC}"
}

print_header() {
    echo ""
    print_message "$BLUE" "════════════════════════════════════════════════════"
    print_message "$BLUE" "$1"
    print_message "$BLUE" "════════════════════════════════════════════════════"
}

check_command() {
    if ! command -v "$1" &> /dev/null; then
        print_message "$RED" "❌ $1 не найден"
        return 1
    fi
    return 0
}

check_sudo() {
    if [ "$EUID" -eq 0 ]; then
        print_message "$YELLOW" "⚠️  Скрипт запущен от root (не рекомендуется)"
    fi
}

detect_package_manager() {
    if command -v dnf &> /dev/null; then
        PKG_MANAGER="dnf"
        print_message "$GREEN" "✅ Обнаружен пакетный менеджер: dnf (Fedora/RHEL)"
    elif command -v yum &> /dev/null; then
        PKG_MANAGER="yum"
        print_message "$GREEN" "✅ Обнаружен пакетный менеджер: yum (RHEL/CentOS)"
    elif command -v apt-get &> /dev/null; then
        PKG_MANAGER="apt"
        print_message "$GREEN" "✅ Обнаружен пакетный менеджер: apt (Ubuntu/Debian)"
    elif command -v apt &> /dev/null; then
        PKG_MANAGER="apt"
        print_message "$GREEN" "✅ Обнаружен пакетный менеджер: apt (Ubuntu/Debian)"
    else
        print_message "$RED" "❌ Не удалось определить пакетный менеджер"
        return 1
    fi
    return 0
}

install_packages_apt() {
    print_message "$BLUE" "Обновление списков пакетов..."
    sudo apt-get update -qq
    
    print_message "$BLUE" "Установка пакетов..."
    sudo apt-get install -y -qq \
        build-essential \
        cmake \
        git \
        libsuitesparse-dev
}

install_packages_dnf() {
    print_message "$BLUE" "Обновление списков пакетов..."
    sudo dnf check-update -q || true
    
    print_message "$BLUE" "Установка пакетов..."
    sudo dnf install -y -q \
        gcc \
        gcc-c++ \
        make \
        cmake \
        git \
        libgomp \
        libgomp-devel \
        suitesparse \
        suitesparse-devel
}

install_packages_yum() {
    print_message "$BLUE" "Обновление списков пакетов..."
    sudo yum check-update -q || true
    
    print_message "$BLUE" "Установка пакетов..."
    sudo yum install -y -q \
        gcc \
        gcc-c++ \
        make \
        cmake \
        git \
        libgomp \
        libgomp-devel \
        suitesparse \
        suitesparse-devel
}

install_graphblas() {
    print_header "🔽 Установка SuiteSparse GraphBLAS"
    
    if [ -f /usr/local/include/GraphBLAS.h ]; then
        print_message "$YELLOW" "⚠️  GraphBLAS уже установлен (пропускаем)"
        return 0
    fi
    
    print_message "$BLUE" "Клонирование репозитория GraphBLAS..."
    if [ -d "$GRAPHBLAS_DIR" ]; then
        rm -rf "$GRAPHBLAS_DIR"
    fi
    git clone --depth 1 "$GRAPHBLAS_REPO" "$GRAPHBLAS_DIR"
    
    print_message "$BLUE" "Сборка GraphBLAS..."
    cd "$GRAPHBLAS_DIR"
    make -j$(nproc) > /dev/null 2>&1
    
    print_message "$BLUE" "Установка GraphBLAS..."
    sudo make install > /dev/null 2>&1
    sudo ldconfig
    
    cd - > /dev/null
    
    print_message "$GREEN" "✅ GraphBLAS установлен"
}

install_lagraph() {
    print_header "🔽 Установка LAGraph"
    
    if [ -f /usr/local/include/LAGraph.h ]; then
        print_message "$YELLOW" "⚠️  LAGraph уже установлен (пропускаем)"
        return 0
    fi
    
    print_message "$BLUE" "Клонирование репозитория LAGraph..."
    if [ -d "$LAGRAPH_DIR" ]; then
        rm -rf "$LAGRAPH_DIR"
    fi
    git clone --depth 1 "$LAGRAPH_REPO" "$LAGRAPH_DIR"
    
    print_message "$BLUE" "Сборка LAGraph..."
    cd "$LAGRAPH_DIR"
    make -j$(nproc) > /dev/null 2>&1
    
    print_message "$BLUE" "Установка LAGraph..."
    sudo make install > /dev/null 2>&1
    sudo ldconfig
    
    cd - > /dev/null
    
    print_message "$GREEN" "✅ LAGraph установлен"
}

check_installation() {
    print_header "✅ Проверка установки"
    
    local all_ok=true
    
    if [ -f /usr/local/include/GraphBLAS.h ]; then
        print_message "$GREEN" "✅ GraphBLAS.h найден"
    else
        print_message "$RED" "❌ GraphBLAS.h не найден"
        all_ok=false
    fi
    
    if [ -f /usr/local/include/LAGraph.h ]; then
        print_message "$GREEN" "✅ LAGraph.h найден"
    else
        print_message "$RED" "❌ LAGraph.h не найден"
        all_ok=false
    fi
    
    if ldconfig -p | grep -q libgraphblas 2>/dev/null; then
        print_message "$GREEN" "✅ libgraphblas найдена"
    else
        print_message "$YELLOW" "⚠️  libgraphblas не найдена в ldconfig"
    fi
    
    if ldconfig -p | grep -q liblagraph 2>/dev/null; then
        print_message "$GREEN" "✅ liblagraph найдена"
    else
        print_message "$YELLOW" "⚠️  liblagraph не найдена в ldconfig"
    fi
    
    if check_command gcc; then
        print_message "$GREEN" "✅ gcc найден: $(gcc --version | head -1)"
    else
        print_message "$RED" "❌ gcc не найден"
        all_ok=false
    fi
    
    if check_command make; then
        print_message "$GREEN" "✅ make найден: $(make --version | head -1)"
    else
        print_message "$RED" "❌ make не найден"
        all_ok=false
    fi
    
    return 0
}

print_header "🔧 SSSP GraphBLAS Benchmark - Установка зависимостей"

check_sudo

print_header "📋 Проверка операционной системы"

OS_NAME=""
OS_VERSION=""

if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS_NAME=$NAME
    OS_VERSION=$VERSION_ID
    print_message "$GREEN" "✅ ОС: $OS_NAME $OS_VERSION"
else
    print_message "$YELLOW" "⚠️  Не удалось определить ОС (продолжаем...)"
fi

print_header "📦 Определение пакетного менеджера"

if ! detect_package_manager; then
    print_message "$RED" "❌ Не удалось определить пакетный менеджер"
    print_message "$YELLOW" "   Поддерживаются: apt (Ubuntu/Debian), dnf/yum (Fedora/RHEL)"
    exit 1
fi

print_header "📦 Установка системных зависимостей"

case $PKG_MANAGER in
    apt)
        install_packages_apt
        ;;
    dnf)
        install_packages_dnf
        ;;
    yum)
        install_packages_yum
        ;;
    *)
        print_message "$RED" "❌ Неподдерживаемый пакетный менеджер: $PKG_MANAGER"
        exit 1
        ;;
esac

if check_command gcc && check_command make && check_command git; then
    print_message "$GREEN" "✅ Системные пакеты установлены"
else
    print_message "$RED" "❌ Ошибка установки системных пакетов"
    exit 1
fi

install_graphblas

install_lagraph

check_installation

print_header "🎉 Установка завершена"

print_message "$GREEN" "Теперь вы можете собрать проект:"
echo "  cd /path/to/project"
echo "  make"
echo ""
print_message "$GREEN" "Запустить тесты:"
echo "  make test"
echo ""
print_message "$GREEN" "Запустить бенчмарк:"
echo "  make run"
echo ""

if [ -n "$OS_NAME" ]; then
    print_message "$BLUE" "ОС: $OS_NAME $OS_VERSION"
fi
print_message "$BLUE" "Пакетный менеджер: $PKG_MANAGER"
echo ""