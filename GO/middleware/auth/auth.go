//go:build !solution

package auth

import (
	"context"
	"errors"
	"fmt"
	"net/http"
	"strings"
)

type typeName string

const userTypeName typeName = "User"

type User struct {
	Name  string
	Email string
}

func ContextUser(ctx context.Context) (*User, bool) {
	val := ctx.Value(userTypeName)
	if val == nil {
		return nil, false
	}
	user, ok := val.(*User)
	if !ok {
		return nil, false
	}
	return user, true
}

var ErrInvalidToken = errors.New("invalid token")

type TokenChecker interface {
	CheckToken(ctx context.Context, token string) (*User, error)
}

func CheckAuth(checker TokenChecker) func(next http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			authHeader := r.Header.Get("Authorization")
			token, err := extractToken(authHeader)
			if err != nil {
				w.WriteHeader(http.StatusUnauthorized)
				return
			}
			userPtr, err := checker.CheckToken(r.Context(), token)
			if errors.Is(err, ErrInvalidToken) {
				w.WriteHeader(http.StatusUnauthorized)
				return
			} else if err != nil {
				w.WriteHeader(http.StatusInternalServerError)
				return
			}
			ctx := context.WithValue(r.Context(), userTypeName, userPtr)
			next.ServeHTTP(w, r.WithContext(ctx))
		})
	}
}

func extractToken(authHeader string) (string, error) {
	if authHeader == "" {
		return "", fmt.Errorf("authorization header is required")
	}
	parts := strings.SplitN(authHeader, " ", 2)
	if len(parts) != 2 || parts[0] != "Bearer" {
		return "", fmt.Errorf("invalid authorization header format")
	}
	return parts[1], nil
}
