# C++ СУБД (Курсовая работа)

### Предварительные требования

1. **Docker Desktop** — [скачать](https://www.docker.com/products/docker-desktop/)
3. **Расширение Dev Containers** для VS Code

### Запуск

1. Клонируйте репозиторий:
   ```bash
   git clone <your-repo-url>
   cd <project-folder>


2. Сборка проекта: 
# Создать папку сборки
mkdir -p build && cd build

# Настроить CMake
cmake ..

# Собрать проект
make -j$(nproc)

# Запустить тесты
ctest --output-on-failure