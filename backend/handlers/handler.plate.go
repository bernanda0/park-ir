package handlers

import (
	"errors"
	"log"
	"net/http"

	"redgate.com/b/db/sqlc"
	"redgate.com/b/utils"
)

func NewPlateIDHandler(l *log.Logger, q *sqlc.Queries, u *AuthedUser) *PlateHandler {
	var c uint = 0
	return &PlateHandler{&Handler{l, q, &c, u}}
}

func (plate_h *PlateHandler) CreatePlateHandler(w http.ResponseWriter, r *http.Request) {
	hp := HandlerParam{w, r, http.MethodPost, plate_h.createPlateId}
	plate_h.h.handleRequest(hp, plate_h.h.u)
}

func (plate_h *PlateHandler) GetPlateIDHandler(w http.ResponseWriter, r *http.Request) {
	hp := HandlerParam{w, r, http.MethodGet, plate_h.getPlateId}
	plate_h.h.handleRequest(hp, plate_h.h.u)
}

func (plate_h *PlateHandler) VerifyPlateHandler(w http.ResponseWriter, r *http.Request) {
	hp := HandlerParam{w, r, http.MethodPost, plate_h.verifyPlateId}
	plate_h.h.handleRequest(hp, nil)
}

func (plate_h *PlateHandler) GetLogs(w http.ResponseWriter, r *http.Request) {
	hp := HandlerParam{w, r, http.MethodGet, plate_h.getLogs}
	plate_h.h.handleRequest(hp, plate_h.h.u)
}

// Implementation
func (ph *PlateHandler) createPlateId(w http.ResponseWriter, r *http.Request) error {
	if err := r.ParseForm(); err != nil {
		http.Error(w, "Error parsing form data", http.StatusBadRequest)
		return err
	}

	// Retrieve form values
	accountID := ph.h.u.UserID
	plateNumber := r.FormValue("plate_number")

	if !utils.PlateNumberIsValid(plateNumber) {
		http.Error(w, "Invalid plate number", http.StatusInternalServerError)
		return errors.New("invalid plate number")
	}
	vID := utils.GeneratePlateID(plateNumber)

	// Create cardParams using retrieved form values
	vehicleParams := sqlc.InsertVehicleParams{
		VID:         vID,
		AccountID:   utils.StringToNullString(accountID),
		PlateNumber: utils.StringToNullString(plateNumber),
	}

	veh, err := ph.h.q.InsertVehicle(r.Context(), vehicleParams)
	if err != nil {
		http.Error(w, "Error inserting data", http.StatusInternalServerError)
		return err
	}

	// after creating ID assume that user is also subscribing
	_, err = ph.h.q.AccountSubscribe(r.Context(), accountID)
	if err != nil {
		http.Error(w, "Error subscribing account", http.StatusInternalServerError)
		return err
	}

	w.WriteHeader(http.StatusCreated)
	toJSON(w, veh)
	return nil
}

func (ph *PlateHandler) getPlateId(w http.ResponseWriter, r *http.Request) error {
	accountID := ph.h.u.UserID
	// Create cardParams using retrieved form values

	plateID, err := ph.h.q.GetPlateID(r.Context(), utils.StringToNullString(accountID))
	if err != nil {
		http.Error(w, "Error getting data/no result", http.StatusInternalServerError)
		return err
	}

	w.WriteHeader(http.StatusOK)
	toJSON(w, plateID)
	return nil
}

func (ph *PlateHandler) verifyPlateId(w http.ResponseWriter, r *http.Request) error {
	if err := r.ParseForm(); err != nil {
		http.Error(w, "Error parsing form data", http.StatusBadRequest)
		return err
	}
	// check if the plate id exist
	vID := r.FormValue("v_id")
	location := r.FormValue("location")
	response, err := ph.h.q.VerifyVehicle(r.Context(), vID)
	if err != nil {
		http.Error(w, "Verifying failed", http.StatusInternalServerError)
		return err
	}

	plate_number := response.PlateNumber.String
	if utils.GeneratePlateID(plate_number) != vID {
		http.Error(w, "Mismatch plate number with the hash value", http.StatusInternalServerError)
		return errors.New("Mismatch")
	}

	logParam := sqlc.GenerateLogsParams{
		AccountID: response.AccountID,
		VID:       utils.StringToNullString(response.VID),
		Location:  utils.StringToNullString(location),
		IpAddress: utils.StringToNullString(r.RemoteAddr),
	}
	ph.h.q.GenerateLogs(r.Context(), logParam)

	w.WriteHeader(http.StatusOK)
	toJSON(w, vID)
	return nil
}

func (ph *PlateHandler) getLogs(w http.ResponseWriter, r *http.Request) error {
	if err := r.ParseForm(); err != nil {
		http.Error(w, "Error parsing form data", http.StatusBadRequest)
		return err
	}

	account_id := r.FormValue("account_id")
	response, err := ph.h.q.GetLogs(r.Context(), utils.StringToNullString(account_id))
	if err != nil {
		http.Error(w, "Failed to get logs", http.StatusInternalServerError)
		return err
	}

	w.WriteHeader(http.StatusOK)
	toJSON(w, response)
	return nil
}
