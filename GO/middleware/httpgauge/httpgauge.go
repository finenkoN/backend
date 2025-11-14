//go:build !solution

package httpgauge

import (
	"fmt"
	"net/http"
	"sort"
	"sync"

	"github.com/go-chi/chi/v5"
)

type Gauge struct {
	stats map[string]int
	mu    sync.Mutex
}

func New() *Gauge {
	return &Gauge{stats: make(map[string]int)}
}

func (g *Gauge) Snapshot() map[string]int {
	s := make(map[string]int)
	g.mu.Lock()
	defer g.mu.Unlock()
	for k, v := range g.stats {
		s[k] = v
	}
	return s
}

// ServeHTTP returns accumulated statistics in text format ordered by pattern.
//
// For example:
//
//	/a 10
//	/b 5
//	/c/{userID} 7
func (g *Gauge) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	w.WriteHeader(http.StatusOK)

	g.mu.Lock()
	defer g.mu.Unlock()

	paths := []string{}
	for pattern := range g.stats {
		paths = append(paths, pattern)
	}
	sort.Strings(paths)

	for _, path := range paths {
		w.Write([]byte(fmt.Sprintf("%s %d\n", path, g.stats[path])))
	}
}

func (g *Gauge) Wrap(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		defer func() {
			path := chi.RouteContext(r.Context()).RoutePattern()
			g.mu.Lock()
			g.stats[path] += 1
			g.mu.Unlock()
		}()
		next.ServeHTTP(w, r)
	})
}
