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


### Как запустить дев контейнер
1. clone репозитория
2. В терминале пишите code . 
3. Потом загрузка, должно создать новое окно, либо обновится старое.
4. Потом F1\Ctrl+Shift+P и выбираешь это Dev Containers: Reopen in Container

### Архитектура проекта
kursach_c_plus/
├── .devcontainer/            # Настройки среды разработки (Docker, VS Code)
│   ├── devcontainer.json
│   └── Dockerfile
│
│
├── docs/                     # Документация по курсовой
│   ├── architecture.md       # Описание архитектуры СУБД
│   └── diagrams/             # Схемы B-дерева, архитектуры хранения
│
├── scripts/                  # Скрипты для автоматизации
│   ├── test_queries.sql      # Тестовые SQL-запросы для пакетного режима
│   └── run_tests.sh          # Скрипт для быстрого прогона всех тестов
│
├── include/                  # Заголовочные файлы (.h / .hpp) — интерфейсы
│   ├── api/                  # Обработка клиентских запросов (сеть, JSON)
│   ├── core/                 # Движок СУБД (Executor, Plan)
│   ├── index/                # Интерфейсы индексов (B-Tree, Hash)
│   ├── parser/               # Лексический и синтаксический анализаторы
│   ├── storage/              # Низкоуровневая работа с диском, Page Manager
│   └── utils/                # Вспомогательные вещи (Logger, Timer)
│
├── src/                      # Исходный код (.cpp) — реализация
│   ├── api/
│   ├── core/
│   ├── index/
│   ├── parser/
│   ├── storage/
│   ├── utils/
|   |__ hello.cpp             #херня под удаление 
│   └── main.cpp              # Точка входа (запуск сервера/CLI, разбор аргументов)
│
├── tests/                    # Модульные и интеграционные тесты
│   ├── CMakeLists.txt        # Локальный скрипт сборки тестов
│   ├── test_index.cpp        # Тесты для B-дерева
│   ├── test_parser.cpp       # Тесты парсера запросов
│   └── test_storage.cpp      # Тесты записи/чтения страниц с диска
│
├── db_data/                  # РАБОЧАЯ ДИРЕКТОРИЯ СУБД (создается при работе программы)
│   ├── global_metadata.json  # Список всех существующих баз данных
│   ├── string_pool.bin       # [Задание 2] Единое хранилище уникальных строк
│   │
│   ├── database_alpha/       # Папка конкретной базы данных
│   │   ├── db.meta           # Список таблиц в этой БД
│   │   │
│   │   └── users_table/      # Папка конкретной таблицы
│   │       ├── schema.meta   # Типы колонок, флаги (NOT_NULL, INDEXED)
│   │       ├── data.bin      # Сами записи (бинарный файл)
│   │       ├── column_id.idx # [Задание 0] Файл индекса B-tree для колонки "id"
│   │       └── history/      # [Задание 1] Снапшоты/логи для REVERT
│   │
│   └── database_beta/        # Другая база данных
│
├── .gitignore                # Исключает build/, db_data/ и системные файлы из Git
├── CMakeLists.txt            # Главный скрипт сборки проекта
└── README.md                 # Инструкция по сборке и архитектуре проекта