#Browser

A production-quality desktop web browser built with C++20, Qt 6, and Qt WebEngine.

---

## Architecture

```
NovaBrowser/
├── main.cpp                      # Application entry point
├── CMakeLists.txt
├── app/
│   ├── browser/
│   │   ├── BrowserEngine         # Profile, WebEngine settings, download routing
│   │   ├── BrowserWindow         # Main window, menus, toolbar, shortcuts
│   │   ├── BrowserTab            # Per-tab view/page wrapper, reader mode
│   │   ├── TabManager            # Tab lifecycle, suspend/restore, closed-tab stack
│   │   ├── HistoryDialog         # Search/delete/clear history UI
│   │   ├── BookmarkDialog        # Folder tree, search, import/export UI
│   │   ├── DownloadDialog        # Live download progress, pause/resume/cancel
│   │   ├── SettingsDialog        # All settings tabs
│   │   └── ClearDataDialog       # Time-range data clearing
│   ├── network/
│   │   ├── HttpClient            # Async HTTP/S with timeout
│   │   ├── DownloadManager       # QWebEngineDownloadRequest wrapper
│   │   ├── CookieManager         # Cookie store integration, third-party blocking
│   │   └── ProxyManager          # System/HTTP/SOCKS5 proxy configuration
│   ├── storage/
│   │   ├── SQLiteDB              # Thread-safe SQLite wrapper with WAL mode
│   │   ├── HistoryManager        # SQLite-backed visit history
│   │   ├── BookmarkManager       # SQLite-backed bookmarks + folders, JSON I/O
│   │   ├── SessionManager        # JSON session save/restore with auto-save timer
│   │   ├── CacheManager          # Disk/memory cache configuration
│   │   └── SettingsManager       # QSettings wrapper with typed accessors
│   ├── security/
│   │   ├── CertificateManager    # SSL certificate inspection and exceptions
│   │   ├── PermissionManager     # Per-origin geo/mic/cam/notification permissions
│   │   └── SafeBrowsing          # Local host blocklist, threat detection hook
│   ├── reader/
│   │   └── ReaderMode            # JS content extractor + styled HTML generator
│   └── utils/
│       └── Logger                # Thread-safe file + stderr logger
├── resources/
│   ├── resources.qrc
│   └── icons/
└── tests/
    ├── test_history.cpp
    ├── test_bookmarks.cpp
    └── test_settings.cpp
```

---

## Dependencies

| Dependency      | Purpose                          |
|-----------------|----------------------------------|
| Qt 6.5+         | Core, Gui, Widgets               |
| Qt WebEngine    | Chromium-based rendering engine  |
| Qt Network      | HTTP client, proxy, SSL          |
| Qt SQL          | SQLite database access           |
| Qt PrintSupport | Page printing                    |
| Qt Concurrent   | Thread pool utilities            |
| OpenSSL 3.x     | TLS/SSL certificate operations   |
| zlib            | Compression                      |
| SQLite          | Bundled via Qt QSQLITE driver    |

---

## Build Requirements

- CMake 3.20+
- C++20 compiler (GCC 12+, Clang 15+, MSVC 2022+)
- Qt 6.5+ with `qtwebengine`, `qtnetwork`, `qtsql`, `qtprintsupport`
- OpenSSL development headers

### Ubuntu / Debian

```bash
sudo apt update
sudo apt install -y \
    cmake ninja-build \
    qt6-base-dev qt6-webengine-dev qt6-tools-dev \
    libssl-dev zlib1g-dev \
    libgl1-mesa-dev
```

### Fedora / RHEL

```bash
sudo dnf install -y \
    cmake ninja-build \
    qt6-qtbase-devel qt6-qtwebengine-devel \
    openssl-devel zlib-devel mesa-libGL-devel
```

### macOS (Homebrew)

```bash
brew install cmake ninja qt@6 openssl@3
export PATH="/opt/homebrew/opt/qt@6/bin:$PATH"
export OPENSSL_ROOT_DIR="/opt/homebrew/opt/openssl@3"
```

### Windows (vcpkg + MSVC)

```powershell
vcpkg install qt6 openssl zlib
```

---

## Build

```bash
# Clone / enter directory
cd NovaBrowser

# Configure (Release)
cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --parallel

# Run
./build/NovaBrowser

# Build with tests
cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

---

## Features

### Navigation
- URL bar with smart input (URL detection vs. search query routing)
- Back / Forward / Reload / Stop / Home
- SSL indicator (🔒 / ⚠) per tab
- Redirect following, HTTPS enforcement

### Tabs
- Unlimited tabs with title + favicon
- Open, close, duplicate, pin, reorder (drag)
- Background tab loading
- Tab suspension (Discarded lifecycle state) for memory savings
- Restore last closed tab (Ctrl+Shift+T)
- Tab context menu (pin, duplicate, close others)

### History
- SQLite storage with visit count and timestamp
- Full-text search
- Per-entry and bulk deletion
- Time-range clearing

### Bookmarks
- Folder tree (unlimited nesting)
- Add/remove/rename/search
- JSON import / export

### Downloads
- Concurrent downloads via QWebEngineDownloadRequest
- Pause / Resume / Cancel
- Progress tracking (bytes received / total)
- Auto-save to Downloads folder

### Cookies
- Persistent + session cookies via QWebEngineCookieStore
- Per-domain viewer and deletion
- Third-party cookie blocking toggle

### Cache
- Disk cache (configurable size, default 512 MB)
- WAL-mode SQLite for all databases
- Clear cache / clear all browsing data dialog

### Session
- Auto-save every 10 seconds (JSON)
- Restore all tabs + active tab index on startup
- Crash recovery (last saved session)

### Reader Mode
- JavaScript content extractor: titles, headings, paragraphs, images, code blocks
- Removes: navigation, sidebars, ads, popups
- Dark mode support
- Estimated reading time

### Security
- HTTPS / SSL validation via QWebEngineProfile
- Certificate inspection (CertificateManager)
- Per-origin permissions: geolocation, microphone, camera, notifications
- Mixed-content detection
- Local host blocklist (SafeBrowsing)
- User-accepted certificate exceptions

### Developer Tools
- Open DevTools panel (F12)
- View page source (Ctrl+U / menu)
- Find in page (Ctrl+F)

### Settings
- Homepage, search engine, download folder
- Privacy: history, cookies, third-party blocking, Do Not Track
- Browser: JavaScript, popups, cache size
- Session restore toggle

### Keyboard Shortcuts

| Shortcut          | Action                  |
|-------------------|-------------------------|
| Ctrl+T            | New tab                 |
| Ctrl+W            | Close tab               |
| Ctrl+Shift+T      | Restore closed tab      |
| Ctrl+L            | Focus URL bar           |
| Ctrl+R / F5       | Reload                  |
| Ctrl+F            | Find in page            |
| Ctrl+H            | History                 |
| Ctrl+D            | Bookmark current page   |
| Ctrl+J            | Downloads               |
| Ctrl+Tab          | Next tab                |
| Ctrl+Shift+Tab    | Previous tab            |
| F11               | Toggle fullscreen       |
| F12               | Developer tools         |
| Alt+Left          | Back                    |
| Alt+Right         | Forward                 |
| Ctrl+0            | Reset zoom              |
| Ctrl++            | Zoom in                 |
| Ctrl+-            | Zoom out                |
| Escape            | Stop loading / close find bar |

---

## Data Storage

All user data is stored under the platform's app data directory:

| Platform | Path                                              |
|----------|---------------------------------------------------|
| Linux    | `~/.local/share/Nova/NovaBrowser/`               |
| macOS    | `~/Library/Application Support/Nova/NovaBrowser/`|
| Windows  | `%APPDATA%\Nova\NovaBrowser\`                    |

Files:
- `nova.db` — browsing history (SQLite, WAL mode)
- `bookmarks.db` — bookmarks and folders (SQLite, WAL mode)
- `session.json` — session auto-save
- `settings.ini` — user preferences
- `nova.log` — application log
- `cache/` — disk HTTP cache
- `storage/` — WebEngine persistent storage (cookies, localStorage, etc.)
- `blocklist.txt` — safe browsing host blocklist (one host per line)

---

## Design Decisions

- **Singleton managers** (`HistoryManager`, `BookmarkManager`, etc.) use instance-level mutexes for thread safety without global state pollution.
- **SQLite WAL mode** is enabled for all databases; this allows concurrent reads without blocking writes.
- **Tab suspension** uses `QWebEnginePage::LifecycleState::Discarded` to free renderer memory for background tabs.
- **Session auto-save** runs on a 10-second QTimer; the last known state survives crashes.
- **Reader Mode** uses a pure-JavaScript extractor injected via `runJavaScript` — no external parsing libraries required.
- **Certificate exceptions** are stored in memory per-session; they do not persist across restarts (security default).
- **Smart pointer ownership**: all heap objects are managed by `unique_ptr` or Qt parent-child ownership; no naked `new` without a parent.
