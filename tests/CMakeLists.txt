cmake_minimum_required(VERSION 3.20)

find_package(Qt6 REQUIRED COMPONENTS Test Core Sql)

# HistoryManager test
qt_add_executable(test_history
    test_history.cpp
    ../app/storage/SQLiteDB.cpp
    ../app/storage/HistoryManager.cpp
    ../app/utils/Logger.cpp
)
target_include_directories(test_history PRIVATE
    ../app/storage
    ../app/utils
)
target_link_libraries(test_history PRIVATE
    Qt6::Test Qt6::Core Qt6::Sql
)
add_test(NAME TestHistory COMMAND test_history)

# BookmarkManager test
qt_add_executable(test_bookmarks
    test_bookmarks.cpp
    ../app/storage/SQLiteDB.cpp
    ../app/storage/BookmarkManager.cpp
    ../app/utils/Logger.cpp
)
target_include_directories(test_bookmarks PRIVATE
    ../app/storage
    ../app/utils
)
target_link_libraries(test_bookmarks PRIVATE
    Qt6::Test Qt6::Core Qt6::Sql
)
add_test(NAME TestBookmarks COMMAND test_bookmarks)

# Settings test
qt_add_executable(test_settings
    test_settings.cpp
    ../app/storage/SettingsManager.cpp
    ../app/utils/Logger.cpp
)
target_include_directories(test_settings PRIVATE
    ../app/storage
    ../app/utils
)
target_link_libraries(test_settings PRIVATE
    Qt6::Test Qt6::Core
)
add_test(NAME TestSettings COMMAND test_settings)
