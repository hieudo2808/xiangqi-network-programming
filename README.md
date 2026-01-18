# Cờ Tướng Online

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

Ứng dụng chơi cờ tướng trực tuyến theo thời gian thực với Server C và Client Desktop (Python + JavaScript).

## Kiến trúc

```
┌─────────────────────────────────────────────────────────────┐
│                    CLIENT (Desktop App)                      │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────────┐  │
│  │   Python    │◄──►│  JavaScript │◄──►│  C Library      │  │
│  │  pywebview  │    │   UI/Logic  │    │  libclient.so   │  │
│  └─────────────┘    └─────────────┘    └────────┬────────┘  │
└─────────────────────────────────────────────────┼───────────┘
                                                  │ TCP
┌─────────────────────────────────────────────────▼───────────┐
│                    SERVER (C Linux)                          │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────────┐  │
│  │   Epoll     │◄──►│  Handlers   │◄──►│   Database      │  │
│  │   Server    │    │   Logic     │    │   (SQL Server)  │  │
│  └─────────────┘    └─────────────┘    └─────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## Cài đặt

### Yêu cầu

- **Server:** Linux, GCC, unixODBC, SQL Server
- **Client:** Python 3.8+, PyQt6, pywebview

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
source venv/bin/activate  # Linux

# Cài dependencies
pip install pywebview qtpy PyQt6 PyQt6-WebEngine

# Build thư viện C client
cd lib && make && cd ..

# Chạy
python launcher.py
```

## Cấu trúc thư mục

```
CoTuongOnline/
├── server/                 # Server C
│   ├── src/               # Source code
│   │   ├── server.c       # Main server (epoll)
│   │   ├── handlers.c     # Message handlers
│   │   ├── match.c        # Game logic
│   │   ├── db.c           # Database
│   │   └── ...
│   ├── include/           # Headers
│   ├── sql/               # SQL scripts
│   └── Makefile
│
├── client/                # Client Desktop
│   ├── pages/             # HTML pages
│   ├── src/               # JavaScript
│   ├── assets/            # CSS, images
│   ├── lib/               # C client library
│   └── launcher.py        # Python launcher
│
└── docs/                  # Documentation
```

## Cách chơi

1. **Mở ứng dụng** - Chạy `python launcher.py`
2. **Kết nối server** - Nhập IP:Port và bấm "Kết nối"
3. **Đăng nhập/Đăng ký** - Tạo tài khoản hoặc đăng nhập
4. **Tìm trận** - Bấm "Tìm trận" hoặc tạo phòng riêng
5. **Chơi** - Di chuyển quân bằng cách click

## Cấu hình

### Biến môi trường

| Biến           | Mặc định    | Mô tả          |
| -------------- | ----------- | -------------- |
| `XIANGQI_HOST` | `127.0.0.1` | Địa chỉ server |
| `XIANGQI_PORT` | `8080`      | Port server    |

### Database

Cập nhật connection string trong `server/src/db.c`:

```c
#define DB_CONNECTION_STRING "DRIVER={ODBC Driver 18 for SQL Server};SERVER=localhost;DATABASE=XiangqiDB;UID=sa;PWD=your_password;TrustServerCertificate=yes"
```

## Giao thức

- **Transport:** TCP Socket
- **Format:** JSON (newline-delimited)
- **Message Types:** 31 loại (login, move, chat, ...)

## License

MIT License - xem file [LICENSE](LICENSE) để biết thêm chi tiết.

## Tác giả

- Đỗ Hoàng Minh Hiếu
- Nguyễn Lưu Tùng Quân

---

<p align="center">
  Made for Network Programming IT4062 Course
</p>
