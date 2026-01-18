#!/usr/bin/env python3
"""
Launcher for Xiangqi Online Game
Uses pywebview to run the frontend and C client library for network communication
"""

import os
import sys
import json
import ctypes
import threading
import time
import atexit
from pathlib import Path

try:
    import webview
except ImportError:
    print("ERROR: pywebview not installed!")
    print("Please run: pip install pywebview")
    sys.exit(1)


class ClientManager:
    """
    Singleton manager for C client library.
    Encapsulates all client state and provides thread-safe operations.
    """
    _instance = None
    _lock = threading.Lock()
    
    def __new__(cls):
        if cls._instance is None:
            with cls._lock:
                if cls._instance is None:
                    cls._instance = super().__new__(cls)
                    cls._instance._initialized = False
        return cls._instance
    
    def __init__(self):
        if self._initialized:
            return
        
        self._initialized = True
        self.client_lib = None
        self.connected = False
        self.message_queue = []
        self.message_queue_lock = threading.Lock()
        self._callback_ref = None
        self._poll_thread = None
        self._stop_event = threading.Event()
        
        # Configuration from environment or defaults
        self.default_host = os.environ.get('XIANGQI_HOST', '127.0.0.1')
        self.default_port = int(os.environ.get('XIANGQI_PORT', '8080'))
        self._lib_path = Path(__file__).parent / "lib" / "libclient.so"
        
        # Auto-reconnect settings
        self.reconnect_timeout = 300  # 5 minutes
        self.reconnect_interval = 5   # Try every 5 seconds
        self._reconnect_thread = None
        self._reconnect_active = False
        self._reconnect_start_time = None
        self._last_host = None
        self._last_port = None
        
        # Register cleanup on exit
        atexit.register(self.cleanup)
    
    def load_library(self) -> bool:
        """Load the C client shared library"""
        if self.client_lib is not None:
            return True
        
        if not self._lib_path.exists():
            raise FileNotFoundError(
                f"C client library not found: {self._lib_path}\n"
                f"Please compile it first: cd lib && make library"
            )
        
        try:
            self.client_lib = ctypes.CDLL(str(self._lib_path))
            self._setup_function_signatures()
            print(f"✓ C client library loaded: {self._lib_path}")
            return True
        except Exception as e:
            print(f"ERROR loading C client library: {e}")
            return False
    
    def _setup_function_signatures(self):
        """Define C function signatures for ctypes"""
        lib = self.client_lib
        
        lib.client_connect.argtypes = [ctypes.c_char_p, ctypes.c_int]
        lib.client_connect.restype = ctypes.c_int
        
        lib.client_disconnect.argtypes = []
        lib.client_disconnect.restype = ctypes.c_int
        
        lib.client_send_json.argtypes = [ctypes.c_char_p]
        lib.client_send_json.restype = ctypes.c_int
        
        lib.client_is_connected.argtypes = []
        lib.client_is_connected.restype = ctypes.c_bool
        
        lib.client_process_messages.argtypes = []
        lib.client_process_messages.restype = ctypes.c_int
        
        # Message callback type
        self._callback_type = ctypes.CFUNCTYPE(None, ctypes.c_char_p)
        lib.client_set_message_callback.argtypes = [self._callback_type]
        lib.client_set_message_callback.restype = None
    
    def _message_handler(self, json_str):
        """Callback function for received messages from C client"""
        try:
            message = json_str.decode('utf-8')
            with self.message_queue_lock:
                self.message_queue.append(message)
        except Exception as e:
            print(f"Error in message handler: {e}")
    
    def _poll_loop(self):
        """Background thread to poll for messages from C client"""
        if not self.client_lib:
            return
        
        # Set up callback
        callback_func = self._callback_type(self._message_handler)
        self.client_lib.client_set_message_callback(callback_func)
        self._callback_ref = callback_func  # Prevent garbage collection
        
        # Poll for messages until stop event is set
        was_connected = False
        while not self._stop_event.is_set():
            if self.client_lib:
                is_connected = self.client_lib.client_is_connected()
                
                if is_connected:
                    was_connected = True
                    self.client_lib.client_process_messages()
                elif was_connected and not self._reconnect_active:
                    # Connection lost - start auto-reconnect
                    print("[ClientManager] Connection lost! Starting auto-reconnect...")
                    self.connected = False
                    was_connected = False
                    self._start_reconnect()
                    
            self._stop_event.wait(0.01)  # 10ms polling interval, interruptible
    
    def _start_reconnect(self):
        """Start the auto-reconnect process"""
        if self._reconnect_active:
            return
        
        self._reconnect_active = True
        self._reconnect_start_time = time.time()
        
        # Notify JS about reconnecting
        with self.message_queue_lock:
            self.message_queue.append(json.dumps({
                "type": "connection_status",
                "payload": {"status": "reconnecting", "timeout": self.reconnect_timeout}
            }))
        
        self._reconnect_thread = threading.Thread(
            target=self._reconnect_loop, daemon=True, name="ReconnectThread"
        )
        self._reconnect_thread.start()
    
    def _reconnect_loop(self):
        """Background thread to attempt reconnection"""
        host = self._last_host or self.default_host
        port = self._last_port or self.default_port
        attempt = 0
        
        while self._reconnect_active and not self._stop_event.is_set():
            elapsed = time.time() - self._reconnect_start_time
            
            # Check timeout
            if elapsed >= self.reconnect_timeout:
                print(f"[Reconnect] Timeout after {self.reconnect_timeout}s")
                self._reconnect_active = False
                with self.message_queue_lock:
                    self.message_queue.append(json.dumps({
                        "type": "connection_status",
                        "payload": {"status": "reconnect_failed", "reason": "timeout"}
                    }))
                return
            
            attempt += 1
            remaining = int(self.reconnect_timeout - elapsed)
            print(f"[Reconnect] Attempt {attempt}, {remaining}s remaining...")
            
            # Notify JS about attempt
            with self.message_queue_lock:
                self.message_queue.append(json.dumps({
                    "type": "connection_status", 
                    "payload": {"status": "reconnecting", "attempt": attempt, "remaining": remaining}
                }))
            
            try:
                host_bytes = host.encode('utf-8')
                result = self.client_lib.client_connect(host_bytes, port)
                
                if result == 0 and self.client_lib.client_is_connected():
                    print(f"[Reconnect] Success after {attempt} attempts!")
                    self.connected = True
                    self._reconnect_active = False
                    
                    with self.message_queue_lock:
                        self.message_queue.append(json.dumps({
                            "type": "connection_status",
                            "payload": {"status": "reconnected", "attempts": attempt}
                        }))
                    return
            except Exception as e:
                print(f"[Reconnect] Attempt {attempt} failed: {e}")
            
            # Wait before next attempt
            self._stop_event.wait(self.reconnect_interval)
    
    def connect(self, host: str = None, port: int = None) -> dict:
        """Connect to C server"""
        if not self.client_lib:
            return {"success": False, "message": "C client library not loaded"}
        
        host = host or self.default_host
        port = port or self.default_port
        
        try:
            host_bytes = host.encode('utf-8')
            result = self.client_lib.client_connect(host_bytes, port)
            
            if result == 0 and self.client_lib.client_is_connected():
                self.connected = True
                self._last_host = host  # Save for reconnect
                self._last_port = port
                self._start_poll_thread()
                return {"success": True, "message": "Connected"}
            else:
                error_messages = {
                    -1: "Socket creation failed",
                    -2: "Invalid IP address",
                    -3: "Connection refused"
                }
                return {"success": False, "message": error_messages.get(result, f"Connection failed (code: {result})")}
        except Exception as e:
            import traceback
            traceback.print_exc()
            return {"success": False, "message": str(e)}
    
    def _start_poll_thread(self):
        """Start the polling thread if not already running"""
        if self._poll_thread is None or not self._poll_thread.is_alive():
            self._stop_event.clear()
            self._poll_thread = threading.Thread(target=self._poll_loop, daemon=True, name="ClientPollThread")
            self._poll_thread.start()
    
    def disconnect(self) -> dict:
        """Disconnect from server"""
        if not self.client_lib or not self.connected:
            return {"success": False, "message": "Not connected"}
        
        try:
            self._stop_event.set()  # Signal poll thread to stop
            if self._poll_thread and self._poll_thread.is_alive():
                self._poll_thread.join(timeout=1.0)
            
            self.client_lib.client_disconnect()
            self.connected = False
            return {"success": True, "message": "Disconnected"}
        except Exception as e:
            return {"success": False, "message": str(e)}
    
    def send(self, json_str: str) -> dict:
        """Send JSON message to server"""
        if not self.client_lib or not self.connected:
            return {"success": False, "message": "Not connected"}
        
        try:
            json_bytes = json_str.encode('utf-8')
            result = self.client_lib.client_send_json(json_bytes)
            
            if result == 0:
                return {"success": True}
            else:
                return {"success": False, "message": f"Send failed (code: {result})"}
        except Exception as e:
            return {"success": False, "message": str(e)}
    
    def is_connected(self) -> bool:
        """Check connection status"""
        if not self.client_lib:
            return False
        try:
            self.connected = self.client_lib.client_is_connected()
            return self.connected
        except:
            return False
    
    def get_messages(self) -> list:
        """Get pending messages from server"""
        with self.message_queue_lock:
            messages = self.message_queue.copy()
            self.message_queue.clear()
        return messages
    
    def cleanup(self):
        """Cleanup resources on shutdown"""
        print("\n[Launcher] Cleaning up...")
        self._stop_event.set()
        if self._poll_thread and self._poll_thread.is_alive():
            self._poll_thread.join(timeout=1.0)
        if self.client_lib and self.connected:
            self.client_lib.client_disconnect()
        print("[Launcher] Cleanup complete")


class ClientAPI:
    """API exposed to JavaScript via pywebview"""
    
    def __init__(self):
        self.manager = ClientManager()
    
    def connect(self, host=None, port=None):
        return self.manager.connect(host, port)
    
    def disconnect(self):
        return self.manager.disconnect()
    
    def send(self, json_str):
        return self.manager.send(json_str)
    
    def is_connected(self):
        return self.manager.is_connected()
    
    def get_messages(self):
        return self.manager.get_messages()


def create_window():
    """Create and configure webview window"""
    base_path = Path(__file__).parent
    html_path = base_path / "index.html"
    
    if not html_path.exists():
        print(f"ERROR: index.html not found at {html_path}")
        sys.exit(1)
    
    api = ClientAPI()
    
    window = webview.create_window(
        title="Cờ Tướng Online - C Client",
        url=str(html_path),
        width=1400,
        height=900,
        min_size=(800, 600),
        js_api=api
    )
    
    return window


def main():
    """Main entry point"""
    print("=" * 60)
    print("Xiangqi Online - C Client Launcher")
    print("=" * 60)
    
    # Get client manager instance
    manager = ClientManager()
    
    # Load C client library
    print("\n[1/3] Loading C client library...")
    if not manager.load_library():
        print("\nERROR: Failed to load C client library!")
        print(f"Make sure you've compiled it: cd lib && make library")
        sys.exit(1)
    
    # Show server configuration
    print(f"\n[2/3] Server configuration:")
    print(f"  Host: {manager.default_host} (env: XIANGQI_HOST)")
    print(f"  Port: {manager.default_port} (env: XIANGQI_PORT)")
    print(f"  (Make sure the C server is running!)")
    
    # Create and start webview
    print(f"\n[3/3] Starting webview...")
    window = create_window()
    
    print("\n" + "=" * 60)
    print("Application started!")
    print("=" * 60)
    print("\nPress Ctrl+C to exit\n")
    
    try:
        webview.start(debug=True)
    except KeyboardInterrupt:
        print("\n\nShutting down...")
    finally:
        manager.cleanup()


if __name__ == "__main__":
    main()
