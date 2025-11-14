//go:build !solution

package main

import (
	"flag"
	"io"
	"log"
	"net/http"
	"net/url"
	"os"
	"regexp"
	"strings"

	"gopkg.in/yaml.v2"
)

type Rule struct {
	Endpoint               string   `yaml:"endpoint"`
	ForbiddenUserAgents    []string `yaml:"forbidden_user_agents"`
	ForbiddenHeaders       []string `yaml:"forbidden_headers"`
	RequiredHeaders        []string `yaml:"required_headers"`
	MaxRequestLengthBytes  int      `yaml:"max_request_length_bytes"`
	MaxResponseLengthBytes int      `yaml:"max_response_length_bytes"`
	ForbiddenResponseCodes []int    `yaml:"forbidden_response_codes"`
	ForbiddenRequestRE     []string `yaml:"forbidden_request_re"`
	ForbiddenResponseRE    []string `yaml:"forbidden_response_re"`
}

type Config struct {
	Rules []Rule `yaml:"rules"`
}

func main() {
	serviceAddr := flag.String("service-addr", "", "адрес защищаемого сервиса")
	confPath := flag.String("conf", "", "путь к .yaml конфигу с правилами")
	addr := flag.String("addr", "", "адрес, на котором будет развёрнут файрвол")
	flag.Parse()

	if *serviceAddr == "" || *confPath == "" || *addr == "" {
		log.Fatal("Все параметры (service-addr, conf, addr) обязательны")
	}

	config, err := loadConfig(*confPath)
	if err != nil {
		log.Fatalf("Ошибка загрузки конфига: %v", err)
	}

	proxy := &Proxy{
		Target: *serviceAddr,
		Config: config,
		Client: &http.Client{},
	}

	log.Printf("Запуск файрвола на %s, защищаем %s", *addr, *serviceAddr)
	log.Fatal(http.ListenAndServe(*addr, proxy))
}

func loadConfig(path string) (*Config, error) {
	data, err := os.ReadFile(path)
	if err != nil {
		return nil, err
	}

	var config Config
	err = yaml.Unmarshal(data, &config)
	if err != nil {
		return nil, err
	}

	return &config, nil
}

type Proxy struct {
	Target string
	Config *Config
	Client *http.Client
}

func (p *Proxy) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	for _, rule := range p.Config.Rules {
		if !strings.HasPrefix(r.URL.Path, rule.Endpoint) {
			continue
		}

		for _, uaPattern := range rule.ForbiddenUserAgents {
			re := regexp.MustCompile(uaPattern)
			if re.MatchString(r.UserAgent()) {
				http.Error(w, "Forbidden", http.StatusForbidden)
				return
			}
		}

		for _, headerPattern := range rule.ForbiddenHeaders {
			parts := strings.SplitN(headerPattern, ":", 2)
			if len(parts) != 2 {
				continue
			}
			key := strings.TrimSpace(parts[0])
			valuePattern := strings.TrimSpace(parts[1])

			if headerValue := r.Header.Get(key); headerValue != "" {
				re := regexp.MustCompile(valuePattern)
				if re.MatchString(headerValue) {
					http.Error(w, "Forbidden", http.StatusForbidden)
					return
				}
			}
		}

		for _, header := range rule.RequiredHeaders {
			if r.Header.Get(header) == "" {
				http.Error(w, "Forbidden", http.StatusForbidden)
				return
			}
		}

		if rule.MaxRequestLengthBytes > 0 && r.ContentLength > int64(rule.MaxRequestLengthBytes) {
			http.Error(w, "Forbidden", http.StatusForbidden)
			return
		}

		bodyBytes, err := io.ReadAll(r.Body)
		if err != nil {
			http.Error(w, "Bad Request", http.StatusBadRequest)
			return
		}
		r.Body = io.NopCloser(strings.NewReader(string(bodyBytes)))

		for _, pattern := range rule.ForbiddenRequestRE {
			re := regexp.MustCompile(pattern)
			if re.Match(bodyBytes) {
				http.Error(w, "Forbidden", http.StatusForbidden)
				return
			}
		}

		targetURL, err := url.Parse(p.Target + r.URL.Path)
		if err != nil {
			http.Error(w, "Bad Gateway", http.StatusBadGateway)
			return
		}

		proxyReq := &http.Request{
			Method: r.Method,
			URL:    targetURL,
			Header: r.Header,
			Body:   io.NopCloser(strings.NewReader(string(bodyBytes))),
		}

		resp, err := p.Client.Do(proxyReq)
		if err != nil {
			http.Error(w, "Bad Gateway", http.StatusBadGateway)
			return
		}
		defer resp.Body.Close()

		for _, code := range rule.ForbiddenResponseCodes {
			if resp.StatusCode == code {
				http.Error(w, "Forbidden", http.StatusForbidden)
				return
			}
		}

		if rule.MaxResponseLengthBytes > 0 && resp.ContentLength > int64(rule.MaxResponseLengthBytes) {
			http.Error(w, "Forbidden", http.StatusForbidden)
			return
		}

		respBodyBytes, err := io.ReadAll(resp.Body)
		if err != nil {
			http.Error(w, "Bad Gateway", http.StatusBadGateway)
			return
		}

		for _, pattern := range rule.ForbiddenResponseRE {
			re := regexp.MustCompile(pattern)
			if re.Match(respBodyBytes) {
				http.Error(w, "Forbidden", http.StatusForbidden)
				return
			}
		}

		for k, v := range resp.Header {
			w.Header()[k] = v
		}
		w.WriteHeader(resp.StatusCode)
		w.Write(respBodyBytes)
		return
	}

	targetURL, err := url.Parse(p.Target + r.URL.Path)
	if err != nil {
		http.Error(w, "Bad Gateway", http.StatusBadGateway)
		return
	}

	proxyReq := &http.Request{
		Method: r.Method,
		URL:    targetURL,
		Header: r.Header,
		Body:   r.Body,
	}

	resp, err := p.Client.Do(proxyReq)
	if err != nil {
		http.Error(w, "Bad Gateway", http.StatusBadGateway)
		return
	}
	defer resp.Body.Close()

	for k, v := range resp.Header {
		w.Header()[k] = v
	}
	w.WriteHeader(resp.StatusCode)
	io.Copy(w, resp.Body)
}
