package utils

import (
	"regexp"
)

// EmailIsValid checks if the provided email is valid using a regular expression.
func EmailIsValid(email string) bool {
	// Define a regular expression pattern for a basic email validation
	// This pattern might need to be adjusted based on your requirements
	emailPattern := `^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,4}$`
	return regexp.MustCompile(emailPattern).MatchString(email)
}

// PasswordIsValid checks if the provided password meets your validation criteria.
func PasswordIsValid(password string) bool {
	// Define your password validation criteria using regular expressions
	// This pattern might need to be adjusted based on your requirements
	passwordPattern := `^[a-zA-Z0-9!@#$%^&*()_+{}[\]:;<>,.?~=-]{6,}$`
	return regexp.MustCompile(passwordPattern).MatchString(password)
}

func PlateNumberIsValid(plate string) bool {
	pattern := `^[A-Z]{1,2}\d{3,4}[A-Z]{1,2}`

	// Compile the regular expressions
	regex1 := regexp.MustCompile(pattern)

	// Check if the plate number matches either format
	return regex1.MatchString(plate)
}
