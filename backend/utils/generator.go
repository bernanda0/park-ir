package utils

import (
	"database/sql"
	"fmt"
	"hash/fnv"
	"strconv"
)

func GeneratePlateID(plate string) string {
	// Initialize a new FNV-1a hash (32-bit)
	h := fnv.New32a()

	// Write the plate data to the hash
	h.Write([]byte(plate))

	// Sum the hash and convert it to a string
	hash := h.Sum32()
	uniqueChars := fmt.Sprintf("%08X", hash) // Convert to an 8-character hexadecimal string

	return "VID" + uniqueChars
}

func NullStringToString(ns sql.NullString) string {
	if ns.Valid {
		return ns.String
	}
	return "" // Handle the case where the value is NULL
}

func StringToNullString(s string) sql.NullString {
	if s != "" {
		return sql.NullString{String: s, Valid: true}
	}
	return sql.NullString{Valid: false}
}

func StringToNullInt(s string) sql.NullInt32 {
	if s != "" {
		value, _ := strconv.ParseInt(s, 10, 32)
		return sql.NullInt32{Int32: int32(value), Valid: true}
	}
	return sql.NullInt32{Valid: false}
}
