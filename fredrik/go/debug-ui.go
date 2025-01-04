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

	// Create a channel for the data
	dataChan := make(chan []byte, 100*1024) // Buffer size of 100kb

	// Start writer goroutine
	go func() {
		for data := range dataChan {
			os.Stdout.Write(data)
		}
	}()

	// Allocate buffer once
	data := make([]byte, 1024*50)

	// Read from socket
	for {
		n, err := conn.Read(data)
		if err != nil {
			log.Println("Client disconnected")
			return
		}

		select {
		case dataChan <- data[:n]:
			// Data sent
		default:
			log.Println("Warning: Dropping debug output - terminal too slow")
			// os.Exit(1)
			dataChan <- data[:n] // This is admittedly pretty retarded.
		}
	}
}
