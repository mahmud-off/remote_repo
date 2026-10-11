# Тапалка

Простая игра-тапалка: фронтенд (HTML/JS + nginx) и бэкенд (Go) как отдельные микросервисы.

## Структура
- `backend/` — Go-сервер с счётчиком кликов (`/count`, `/tap`, `/reset`)
- `frontend/` — статическая страница, ходит в бэкенд по HTTP
- `docker-compose.yml` — сборка и запуск обоих сервисов

## Запуск
```bash
docker compose up --build
```

Открыть: http://localhost:3000

Бэкенд доступен на http://localhost:8080
