package main

import (
	"log"
	"net"
	"os"
	"os/signal"
	"syscall"
)

const socketPath = "/tmp/debug_socket"

func main() {
	// Clean up existing socket file
	os.Remove(socketPath)

	// Create Unix domain socket listener
	listener, err := net.Listen("unix", socketPath)
	if err != nil {
		log.Fatal("Listen error: ", err)
	}
	defer listener.Close()

	log.Printf("Listening on %s", socketPath)

	// Handle Ctrl+C gracefully
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
	go func() {
		<-sigChan
		log.Println("\nCleaning up...")
		listener.Close()
		os.Remove(socketPath)
		os.Exit(0)
	}()

	// Accept one connection
	conn, err := listener.Accept()
	if err != nil {
		log.Fatal("Accept error: ", err)
	}
	log.Println("Client connected")

	// Just read and discard data
	buffer := make([]byte, 4096)
	for {
		_, err := conn.Read(buffer)
		if err != nil {
			log.Println("Client disconnected")
			return
		}
		// Discard data - no processing
	}
} 