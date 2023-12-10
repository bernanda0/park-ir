# Use an official Golang runtime as a parent image
FROM golang:1.21

# Set the working directory in the container
WORKDIR /app

# Copy the local application code to the container
COPY . .

# Build the application
RUN go build -o my-backend

# Expose the port the application runs on
EXPOSE 4444

# Run the application
CMD ["./my-backend"]
