# Cờ Tướng Online

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

Ứng dụng chơi cờ tướng trực tuyến theo thời gian thực với Server C và Client Desktop (Python + JavaScript).

## Cài đặt

### Yêu cầu

- **Server:** Linux, GCC, unixODBC, SQL Server
- **Client:** Python, PyQt6, pywebview

### Server

```bash
# Cài dependencies (Ubuntu/Debian)
cd server
make install-deps

# Build
make

# Chạy (port 8080)
./bin/server 8080
```

### Client

```bash
cd client

# Tạo virtual environment
python -m venv venv
source venv/bin/activate

# Cài dependencies
pip install pywebview qtpy PyQt6 PyQt6-WebEngine

# Build thư viện C client
cd lib && make && cd ..

# Chạy
python launcher.py
```

### Database

Cập nhật connection string trong `server/src/db.c`:

```c
#define DB_CONNECTION_STRING "DRIVER={ODBC Driver 18 for SQL Server};SERVER=localhost;DATABASE=XiangqiDB;UID=sa;PWD=your_password;TrustServerCertificate=yes"
```