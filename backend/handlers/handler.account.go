package handlers

import (
	"log"
	"net/http"

	"redgate.com/b/db/sqlc"
	"redgate.com/b/utils"
)

func NewAccountHandler(l *log.Logger, q *sqlc.Queries, u *AuthedUser) *AccountHandler {
	var c uint = 0
	return &AccountHandler{&Handler{l, q, &c, u}}
}

func (ah *AccountHandler) CreateAccountH(w http.ResponseWriter, r *http.Request) {
	hp := HandlerParam{w, r, http.MethodPost, ah.createAccount}
	ah.h.handleRequest(hp, ah.h.u)
}

func (ah *AccountHandler) createAccount(w http.ResponseWriter, r *http.Request) error {
	// Parse form data
	if err := r.ParseForm(); err != nil {
		http.Error(w, "Error parsing form data", http.StatusBadRequest)
		return err
	}

	// Retrieve form values
	username := r.FormValue("username")
	email := r.FormValue("email")
	password := r.FormValue("password")
	hashedPassword, _ := utils.HashPassword(password)

	// Create accountParams using retrieved form values
	accountParams := sqlc.CreateAccountParams{
		Username:     username,
		Email:        email,
		PasswordHash: hashedPassword, // Don't forget to hash the password
	}

	account, err := ah.h.q.CreateAccount(r.Context(), accountParams)
	if err != nil {
		http.Error(w, "Error creating account", http.StatusInternalServerError)
		return err
	}

	w.WriteHeader(http.StatusCreated)
	toJSON(w, account)
	return nil
}
