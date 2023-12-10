package handlers

import (
	"log"
	"net/http"

	"redgate.com/b/db/sqlc"
	"redgate.com/b/utils"
)

func NewWalletHandler(l *log.Logger, q *sqlc.Queries, u *AuthedUser) *WalletHandler {
	var c uint = 0
	return &WalletHandler{&Handler{l, q, &c, u}}
}

func (wallet_h *WalletHandler) TopUpHandler(w http.ResponseWriter, r *http.Request) {
	hp := HandlerParam{w, r, http.MethodPost, wallet_h.topUpHandler}
	wallet_h.h.handleRequest(hp, wallet_h.h.u)
}

func (wallet_h *WalletHandler) GetBalance(w http.ResponseWriter, r *http.Request) {
	hp := HandlerParam{w, r, http.MethodGet, wallet_h.getBalance}
	wallet_h.h.handleRequest(hp, wallet_h.h.u)
}

func (wh *WalletHandler) topUpHandler(w http.ResponseWriter, r *http.Request) error {
	if err := r.ParseForm(); err != nil {
		http.Error(w, "Error parsing form data", http.StatusBadRequest)
		return err
	}

	// Retrieve form values
	amount := r.FormValue("amount")
	accountID := wh.h.u.UserID
	walletID, _ := wh.h.q.GetWalletID(r.Context(), utils.StringToNullString(accountID))
	wh.h.l.Println("Will top up to " + walletID + " " + accountID)
	topUpParams := sqlc.TopUpParams{
		Amount:   utils.StringToNullInt(amount),
		WalletID: walletID,
	}
	veh, err := wh.h.q.TopUp(r.Context(), topUpParams)
	if err != nil {
		http.Error(w, "Error Top Up Data", http.StatusInternalServerError)
		return err
	}

	wh.h.l.Println("Success top up to user id ", accountID)
	w.WriteHeader(http.StatusOK)
	toJSON(w, veh)
	return nil
}

func (wh *WalletHandler) getBalance(w http.ResponseWriter, r *http.Request) error {
	accountID := wh.h.u.UserID

	balance, err := wh.h.q.GetBalance(r.Context(), utils.StringToNullString(accountID))
	if err != nil {
		http.Error(w, "Error Top Up Data", http.StatusInternalServerError)
		return err
	}

	w.WriteHeader(http.StatusOK)
	toJSON(w, balance.Int32)
	return nil
}
