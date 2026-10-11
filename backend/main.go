package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"sync"
	"time"
)

var (
	mu    sync.Mutex
	score int64 = 0
	logs  []string
)

type countResponse struct {
	Score int64 `json:"score"`
}

// addLog сохраняет строку лога в памяти и печатает её в stdout контейнера
func addLog(format string, args ...any) {
	line := fmt.Sprintf("%s %s", time.Now().Format("2006-01-02 15:04:05"), fmt.Sprintf(format, args...))
	mu.Lock()
	logs = append(logs, line)
	if len(logs) > 1000 {
		logs = logs[len(logs)-1000:]
	}
	mu.Unlock()
	log.Println(line)
}

func getCount(w http.ResponseWriter, r *http.Request) {
	mu.Lock()
	s := score
	mu.Unlock()
	addLog("GET /count from %s -> score=%d", r.RemoteAddr, s)
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(countResponse{Score: s})
}

func increment(w http.ResponseWriter, r *http.Request) {
	mu.Lock()
	score++
	s := score
	mu.Unlock()
	addLog("POST /tap from %s -> score=%d", r.RemoteAddr, s)
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(countResponse{Score: s})
}

func reset(w http.ResponseWriter, r *http.Request) {
	mu.Lock()
	score = 0
	mu.Unlock()
	addLog("POST /reset from %s -> score=0", r.RemoteAddr)
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(countResponse{Score: 0})
}

// getLogs — ручка для просмотра логов: /logs возвращает JSON-массив строк
func getLogs(w http.ResponseWriter, r *http.Request) {
	mu.Lock()
	out := make([]string, len(logs))
	copy(out, logs)
	mu.Unlock()
	addLog("GET /logs from %s", r.RemoteAddr)
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(out)
}

func cors(next http.HandlerFunc) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		w.Header().Set("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
		if r.Method == http.MethodOptions {
			return
		}
		next(w, r)
	}
}

func main() {
	http.HandleFunc("/count", cors(getCount))
	http.HandleFunc("/tap", cors(increment))
	http.HandleFunc("/reset", cors(reset))
	http.HandleFunc("/logs", cors(getLogs))

	log.Println("backend listening on :8080")
	log.Fatal(http.ListenAndServe(":8080", nil))
}
