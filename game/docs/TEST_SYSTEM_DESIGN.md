# Дизайн системы автоматизированного тестирования

## 1. Анализ текущих проблем

Текущая система тестирования использует bash-скрипты, которые отправляют TCP-команды через `nc`:

```bash
echo "KEY_PRESS 257" | nc -w 2 localhost 9999
sleep 0.35
echo "SCREENSHOT test.png" | nc -w 2 localhost 9999
```

**Проблемы:**

1. **Одно соединение = одна команда.** `command_server_respond()` закрывает сокет после каждого ответа (`command_server.h:193`). Каждая команда — новый TCP handshake.

2. **`sleep` не привязан к кадрам.** Реальное время и игровые кадры не синхронизированы. `sleep 0.35` может соответствовать 20 или 22 кадрам в зависимости от нагрузки системы.

3. **Нет запросов состояния.** Невозможно узнать текущий `game.state`, `money`, `hits`, `level` — только скриншоты.

4. **Нет условной логики.** Нельзя дождаться перехода в определённое состояние или проверить утверждение.

5. **Нет батчинга.** Каждая команда требует отдельного вызова `nc`.

## 2. Выбранный подход: Скриптовый движок (Вариант D+)

**Простой построчный скриптовый файл** загружается одной TCP-командой `SCRIPT <файл>`, выполняется покадрово внутри игры.

### Почему не Lisp / Forth / другие

| Подход | Плюсы | Минусы | Вердикт |
|--------|-------|--------|---------|
| Lisp-подобный | Вложенность, условия | Сложный парсер (~500 строк) | Избыточно |
| Forth/стек | Компактный | Непривычный синтаксис | Непрактично |
| Расширенный TCP | Минимальные изменения | Bash-скрипты всё ещё ненадёжны | Полумера |
| **Построчный скрипт** | **Тривиальный парсер, кадрово-точный** | Нет вложенности | **Достаточно** |

Построчный скрипт — это минимальное решение, покрывающее все потребности. Одна команда на строку, одна строка на кадр, `WAIT` считает кадры, `ASSERT` проверяет состояние.

## 3. Протокол и команды

### 3.1 Новая TCP-команда

```
SCRIPT <путь_к_файлу>
```

Загружает файл, парсит все строки в очередь, запускает покадровое выполнение. TCP-соединение остаётся открытым до конца скрипта. В конце отправляется итоговый отчёт.

### 3.2 Команды скрипта

| Команда | Формат | Описание |
|---------|--------|----------|
| `KEY` | `KEY <код>` | Симулировать нажатие клавиши |
| `MOUSE` | `MOUSE <кнопка>` | Симулировать нажатие мыши |
| `MOVE` | `MOVE <x> <y>` | Переместить мышь |
| `SHOT` | `SHOT <файл>` | Скриншот |
| `WAIT` | `WAIT <кадры>` | Пауза на N кадров (60 кадров ≈ 1 сек) |
| `WAIT_STATE` | `WAIT_STATE <состояние> [таймаут]` | Ждать перехода в состояние (таймаут в кадрах, по умолчанию 600) |
| `ASSERT_STATE` | `ASSERT_STATE <состояние>` | Проверить текущее состояние |
| `ASSERT_GE` | `ASSERT_GE <поле> <значение>` | Проверить поле >= значение |
| `ASSERT_LE` | `ASSERT_LE <поле> <значение>` | Проверить поле <= значение |
| `ASSERT_EQ` | `ASSERT_EQ <поле> <значение>` | Проверить поле == значение |
| `GET` | `GET <поле>` | Отправить значение поля клиенту |
| `LOG` | `LOG <текст>` | Напечатать сообщение в отчёт |
| `QUIT` | `QUIT` | Завершить игру |
| `#` | `# комментарий` | Комментарий (игнорируется) |

### 3.3 Имена состояний

`LOGO`, `MENU`, `TUTOR1`, `TUTOR2`, `TUTOR3`, `NIGHT`, `DAY`, `WIN`, `OVER`

### 3.4 Запрашиваемые поля

| Поле | Тип | Описание |
|------|-----|----------|
| `state` | string | Текущее состояние |
| `money` | int | Деньги |
| `hits` | int | Хиты |
| `level` | int | Уровень |
| `level_time` | float | Время в раунде |
| `cur_row` | int | Текущая строка |
| `cur_col` | int | Текущий столбец |
| `creatures` | int | Количество активных существ |
| `club_bought` | bool | Клуб куплен |

## 4. Структуры данных

```c
#define SCRIPT_MAX_LINES 1024
#define SCRIPT_MAX_LINE_LEN 256

typedef struct {
    char lines[SCRIPT_MAX_LINES][SCRIPT_MAX_LINE_LEN];
    int line_count;          // всего строк
    int current_line;        // текущая строка
    int wait_remaining;      // оставшиеся кадры ожидания
    int wait_state_target;   // целевое состояние для WAIT_STATE (-1 = нет)
    int wait_state_timeout;  // таймаут WAIT_STATE
    bool active;             // скрипт выполняется
    int pass_count;          // пройденные ASSERT
    int fail_count;          // проваленные ASSERT
    int client_fd;           // TCP-соединение для отчёта
} ScriptRunner;
```

## 5. Движок выполнения

### 5.1 Жизненный цикл

```
1. TCP: клиент подключается, отправляет "SCRIPT test.sszb\n"
2. Парсинг: файл читается, строки загружаются в ScriptRunner
3. Выполнение: каждый кадр — одна строка (или декремент WAIT)
4. Завершение: отправить отчёт клиенту, закрыть соединение
```

### 5.2 Интеграция в игровой цикл

В `command_server_poll()` добавляется проверка:

```c
Command command_server_poll(void) {
    // Если скрипт активен — выполняем следующую строку
    if (script_runner.active) {
        return script_runner_tick(&script_runner);
    }
    // Иначе — обычная TCP-логика
    ...
}
```

### 5.3 Механика WAIT

```c
Command script_runner_tick(ScriptRunner *sr) {
    Command cmd = {.type = CMD_NONE};

    // Если ждём кадры
    if (sr->wait_remaining > 0) {
        sr->wait_remaining--;
        return cmd;  // CMD_NONE = пропуск кадра
    }

    // Если ждём состояние
    if (sr->wait_state_target >= 0) {
        // Проверяется в main.c после получения CMD_NONE
        // Таймаут декрементируется, при достижении 0 — FAIL
        sr->wait_state_timeout--;
        if (sr->wait_state_timeout <= 0) {
            sr->fail_count++;
            log_fail("WAIT_STATE timeout");
            sr->wait_state_target = -1;
        }
        return cmd;
    }

    // Следующая строка
    if (sr->current_line >= sr->line_count) {
        script_runner_finish(sr);
        return cmd;
    }

    return script_parse_line(sr->lines[sr->current_line++]);
}
```

### 5.4 Обработка ASSERT и GET в main.c

```c
case CMD_ASSERT_STATE:
    if (game.state == frame_cmd.assert_state) {
        script_runner.pass_count++;
        script_respond("PASS");
    } else {
        script_runner.fail_count++;
        script_respond("FAIL: expected %s, got %s", ...);
    }
    break;

case CMD_GET:
    script_respond("money=%d", game.money);
    break;
```

### 5.5 WAIT_STATE в main.c

После каждого `state_update()`:

```c
if (script_runner.active && script_runner.wait_state_target >= 0) {
    if (game.state == script_runner.wait_state_target) {
        script_runner.wait_state_target = -1;
        script_respond("OK state reached");
    }
}
```

## 6. Формат отчёта

По завершении скрипта клиенту отправляется:

```
=== SCRIPT REPORT ===
PASS: 12
FAIL: 0
TOTAL: 12
---
[PASS] line 5: ASSERT_STATE NIGHT
[PASS] line 8: ASSERT_GE money 10
...
=== END REPORT ===
```

## 7. Пример тестового скрипта

```bash
# test_full_cycle.sszb
# Полный цикл: Logo → Menu → Tutorials → Night → Day → Night 2

# Logo → Menu (logo длится ~60 кадров = 1 секунда)
LOG Phase 1: Logo to Menu
WAIT 90
ASSERT_STATE MENU
SHOT 01_menu.png

# Menu → Tutorials → Night
LOG Phase 2: Menu to Night
KEY 257
WAIT 2
ASSERT_STATE TUTOR1
KEY 257
WAIT 2
ASSERT_STATE TUTOR2
KEY 257
WAIT 2
ASSERT_STATE TUTOR3
KEY 257
WAIT 2
ASSERT_STATE NIGHT
SHOT 02_night_start.png

# Night 1: fire weapon
LOG Phase 3: Night gameplay
ASSERT_EQ level 1
ASSERT_EQ money 10
KEY 32
WAIT 60
SHOT 03_night_fire.png

# Wait for enemies
WAIT 240
ASSERT_GE creatures 1
SHOT 04_night_enemies.png

# Skip to day via cheat
LOG Phase 4: Skip to Day
KEY 76
WAIT 5
ASSERT_STATE DAY
SHOT 05_day_start.png

# Buy a room
LOG Phase 5: Day shopping
KEY 262
WAIT 2
KEY 49
WAIT 2
ASSERT_GE money 0
SHOT 06_day_bought.png

# Advance to Night 2
LOG Phase 6: Night 2
KEY 257
WAIT_STATE NIGHT 120
ASSERT_STATE NIGHT
ASSERT_EQ level 2
SHOT 07_night2.png

# Force game over
LOG Phase 7: Game Over
KEY 80
KEY 80
KEY 80
KEY 80
KEY 80
WAIT 30
ASSERT_STATE OVER
SHOT 08_gameover.png

# Restart
LOG Phase 8: Restart
KEY 257
WAIT 90
ASSERT_STATE MENU
ASSERT_EQ money 10
SHOT 09_restart.png

LOG All phases complete
QUIT
```

## 8. Файлы для изменения

| Файл | Изменение | Объём |
|------|-----------|-------|
| `src/command_server.h` | Добавить `ScriptRunner`, `script_runner_tick()`, парсинг `SCRIPT`, не закрывать сокет во время скрипта | +200 строк |
| `src/main.c` | Обработка `CMD_ASSERT_*`, `CMD_GET`, `CMD_WAIT_STATE`, проверка `wait_state_target` | +80 строк |
| `src/game_types.h` | Новые `CommandType` значения: `CMD_SCRIPT`, `CMD_ASSERT_STATE`, `CMD_ASSERT_GE/LE/EQ`, `CMD_GET`, `CMD_LOG`, `CMD_WAIT` | +20 строк |
| `tests/test_full_cycle.sszb` | Тестовый скрипт из примера выше | ~60 строк |

**Итого: ~300 строк нового кода + ~60 строк тестов.**

## 9. Запуск тестов

```bash
# Собрать и запустить игру
make build && make run &

# Подождать TCP-сервер
sleep 2

# Запустить тестовый скрипт (одна команда, одно соединение)
echo "SCRIPT tests/test_full_cycle.sszb" | nc localhost 9999

# nc остаётся подключённым, получает отчёт по завершении
```

Или интегрировать в Makefile:

```makefile
.PHONY: test
test: build
	./build/Game &
	sleep 2
	echo "SCRIPT tests/test_full_cycle.sszb" | nc localhost 9999
	kill %1
```

## 10. Расширения на будущее

- **LOOP N / ENDLOOP** — повторение блока N раз
- **IF_STATE / ENDIF** — условное выполнение
- **WAIT_MONEY_GE <N>** — ждать накопления денег
- **SPEED <множитель>** — ускорение игры для тестов
- **SEED <число>** — фиксация `srand` для воспроизводимых тестов
