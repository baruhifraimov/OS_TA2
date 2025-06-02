#pragma once

#define MAXDATASIZE 100 // Maximum number of bytes we can receive in one recv() call and in send()
#define MAX_CLIENTS 100  // Maximum number of clients we'll handle

// Handlers for the message process
#define TCP_HANDLE 0
#define UDP_HANDLE 1
#define KEYBOARD_HANDLE 2