//go:build !solution

package requestlog

import (
	"context"
	"net/http"
	"time"

	"go.uber.org/zap"
)

type requestIDKey struct{}

func Log(logger *zap.Logger) func(http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			requestID := time.Now().Format("20060102150405") + "-" + randomString(8)

			ctx := context.WithValue(r.Context(), requestIDKey{}, requestID)
			r = r.WithContext(ctx)

			logger.Info("request started",
				zap.String("path", r.URL.Path),
				zap.String("method", r.Method),
				zap.String("request_id", requestID),
			)

			wrapped := &responseWriterWrapper{
				ResponseWriter: w,
				statusCode:     http.StatusOK,
			}

			start := time.Now()

			defer func() {
				if err := recover(); err != nil {
					logger.Error("request panicked",
						zap.String("path", r.URL.Path),
						zap.String("method", r.Method),
						zap.String("request_id", requestID),
						zap.Any("panic", err),
						zap.Duration("duration", time.Since(start)),
					)
					panic(err)
				}

				logger.Info("request finished",
					zap.String("path", r.URL.Path),
					zap.String("method", r.Method),
					zap.String("request_id", requestID),
					zap.Int("status_code", wrapped.statusCode),
					zap.Duration("duration", time.Since(start)),
				)
			}()

			next.ServeHTTP(wrapped, r)
		})
	}
}

type responseWriterWrapper struct {
	http.ResponseWriter
	statusCode int
}

func (rw *responseWriterWrapper) WriteHeader(code int) {
	rw.statusCode = code
	rw.ResponseWriter.WriteHeader(code)
}

func randomString(length int) string {
	const charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
	b := make([]byte, length)
	for i := range b {
		b[i] = charset[int(time.Now().UnixNano())%len(charset)]
	}
	return string(b)
}
