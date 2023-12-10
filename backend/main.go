package main

import (
	"context"
	"crypto/tls"
	"flag"
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"

	"github.com/joho/godotenv"
	"github.com/rs/cors"
	"redgate.com/b/db"
	"redgate.com/b/db/sqlc"
	"redgate.com/b/handlers"
	"redgate.com/b/token"
)

func main() {
	l := log.New(os.Stdout, "RED-GATE-SERVER-", log.LstdFlags)
	ctx := context.Background()
	certFile := flag.String("certfile", "park-ir-be.site+4.pem", "certificate PEM file")
	keyFile := flag.String("keyfile", "park-ir-be.site+4-key.pem", "key PEM file")
	flag.Parse()

	env := os.Getenv("APP_ENV")
	if env == "" {
		env = "remote"
	}
	// load env
	err := godotenv.Load(".env." + env)
	if err != nil {
		l.Fatalf("Error reding the .env %s", err)
	}

	// CRUD
	db, queries := db.Instantiate(l)
	if db == nil || queries == nil {
		l.Println("Exiting due to database connection error")
		return
	}
	defer db.Close()

	server := &http.Server{
		Addr:        "0.0.0.0:" + os.Getenv("PORT"),
		Handler:     defineMultiplexer(l, queries),
		IdleTimeout: 30 * time.Second,
		ReadTimeout: time.Second,
		TLSConfig: &tls.Config{
			MinVersion:               tls.VersionTLS12,
			PreferServerCipherSuites: true,
		},
	}

	// now the startServer is run by a routine
	go startServer(server, l, *certFile, *keyFile)

	// inorder to block the routine, we might use a channel (we can use wait group also)
	shut := make(chan os.Signal, 1)
	signal.Notify(shut, syscall.SIGINT, syscall.SIGTERM)

	<-shut // Block until a signal is received

	timeout_ctx, cancel := context.WithTimeout(ctx, 30*time.Second)
	defer cancel()

	stopServer(server, l, &timeout_ctx, &cancel)
}

func startServer(s *http.Server, l *log.Logger, cert string, key string) {
	l.Println("🔥 Server is running on", s.Addr)

	err := s.ListenAndServeTLS(cert, key)
	if err != nil && err != http.ErrServerClosed {
		l.Fatalln("Server is failed due to", err)
	}
}

func stopServer(s *http.Server, l *log.Logger, ctx *context.Context, cancel *context.CancelFunc) {
	l.Println("💅 Shutting down the server")
	s.Shutdown(*ctx)
	c := *cancel
	c()
}

func defineMultiplexer(l *log.Logger, q *sqlc.Queries) http.Handler {
	var u handlers.AuthedUser

	// reference to the handler
	hello_handler := handlers.NewHello(l)
	token, err := token.NewPasetoMaker(os.Getenv("PASETO_KEY"))
	if err != nil {
		log.Fatal("Failed creating Paseto token")
	}
	auth_handler := handlers.NewAuthHandler(l, q, &u, &token)
	token_handler := handlers.NewTokenHandler(l, q, &u, &token)
	plate_handler := handlers.NewPlateIDHandler(l, q, &u)

	// handle multiplexer
	mux := http.NewServeMux()
	mux.Handle("/", hello_handler)

	mux.HandleFunc("/auth/login", auth_handler.Login)
	mux.HandleFunc("/auth/signup", auth_handler.Signup)
	mux.HandleFunc("/auth/renewToken", token_handler.RenewToken)

	mux.HandleFunc("/plate/create", plate_handler.CreatePlateHandler)
	mux.HandleFunc("/plate/getID", plate_handler.GetPlateIDHandler)
	mux.HandleFunc("/plate/verify", plate_handler.VerifyPlateHandler)
	mux.HandleFunc("/logs", plate_handler.GetLogs)

	corsMiddleware := cors.AllowAll().Handler

	handler := corsMiddleware(mux)

	return handler
}
