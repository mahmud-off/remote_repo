package main

import (
	"encoding/json"
	"log"
	"net/http"
	"sync"
)

var (
	mu     sync.Mutex
	score  int64 = 0
)

type countResponse struct {
	Score int64 `json:"score"`
}

func getCount(w http.ResponseWriter, _ *http.Request) {
	mu.Lock()
	defer mu.Unlock()
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(countResponse{Score: score})
}

func increment(w http.ResponseWriter, _ *http.Request) {
	mu.Lock()
	score++
	s := score
	mu.Unlock()
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(countResponse{Score: s})
}

func reset(w http.ResponseWriter, _ *http.Request) {
	mu.Lock()
	score = 0
	mu.Unlock()
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(countResponse{Score: 0})
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

	log.Println("backend listening on :8080")
	log.Fatal(http.ListenAndServe(":8080", nil))
}
