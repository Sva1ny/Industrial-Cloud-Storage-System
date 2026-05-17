cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force UTF-8 source encoding for all compilers
if(MSVC)
    add_compile_options(/utf-8)
else()
    add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Multimedia MultimediaWidgets)

# ── Find poppler-cpp (PDF rendering) ──
find_library(POPPLER_CPP_LIBRARY poppler-cpp
    HINTS /opt/homebrew/lib /usr/local/lib /usr/lib
)
find_path(POPPLER_CPP_INCLUDE_DIR poppler/cpp/poppler-document.h
    HINTS /opt/homebrew/include /usr/local/include /usr/include
)
if(NOT POPPLER_CPP_LIBRARY OR NOT POPPLER_CPP_INCLUDE_DIR)
    message(FATAL_ERROR "poppler-cpp not found. Install with: brew install poppler")
endif()

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/network/NetworkManager.cpp
    src/pages/SignUpPage.cpp
    src/pages/SignInPage.cpp
    src/pages/HomePage.cpp
    src/pages/TrashPage.cpp
    src/pages/ShareDialog.cpp
    src/pages/ShareListPage.cpp
    src/pages/TransferPanel.cpp
    src/pages/TransferListPage.cpp
    src/preview/PreviewDialog.cpp
    src/preview/ImagePreviewWidget.cpp
    src/preview/VideoPreviewWidget.cpp
    src/preview/PdfPreviewWidget.cpp
    src/preview/TextPreviewWidget.cpp
)

set(HEADERS
    src/MainWindow.h
    src/network/ApiEndpoints.h
    src/network/NetworkManager.h
    src/pages/SignUpPage.h
    src/pages/SignInPage.h
    src/pages/HomePage.h
    src/pages/TrashPage.h
    src/pages/ShareDialog.h
    src/pages/ShareListPage.h
    src/pages/TransferPanel.h
    src/pages/TransferListPage.h
    src/models/UserInfo.h
    src/models/FileItem.h
    src/styles/ThemeManager.h
    src/preview/PreviewDialog.h
    src/preview/ImagePreviewWidget.h
    src/preview/VideoPreviewWidget.h
    src/preview/PdfPreviewWidget.h
    src/preview/TextPreviewWidget.h
)

add_executable(CloudDiskClient ${SOURCES} ${HEADERS})

target_link_libraries(CloudDiskClient PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    ${POPPLER_CPP_LIBRARY}
)

if(APPLE)
    set_target_properties(CloudDiskClient PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.lee1y.clouddisk"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
    )
endif()

target_include_directories(CloudDiskClient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${POPPLER_CPP_INCLUDE_DIR}
)
