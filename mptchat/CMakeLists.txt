cmake_minimum_required(VERSION 4.0)
project(mptchat)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Multimedia)

# 源文件是 UTF-8（无 BOM），显式告诉 MSVC，避免中文串被按 GBK 解析出 C2001/C4819
if(MSVC)
    add_compile_options(/utf-8)
endif()

add_executable(mptchat main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        res.qrc
        logindialog.cpp
        logindialog.h
        logindialog.ui
        registerdialog.cpp
        registerdialog.h
        registerdialog.ui
        global.cpp
        global.h
        singleton.h
        HttpMgr.cpp
        HttpMgr.h
        TimerBtn.cpp
        TimerBtn.h
        ClickedLabel.cpp
        ClickedLabel.h
        resetdialog.cpp
        resetdialog.h
        resetdialog.ui
        TCPMgr.cpp
        TCPMgr.h
        UserMgr.cpp
        UserMgr.h
        chatdialog.cpp
        chatdialog.h
        chatdialog.ui
        CustomizeEdit.cpp
        CustomizeEdit.h
        ChatUserList.cpp
        ChatUserList.h
        ListItemBase.cpp
        ListItemBase.h
        chatuserwid.cpp
        chatuserwid.h
        chatuserwid.ui
        chatpage.cpp
        chatpage.h
        chatpage.ui
        chatview.cpp
        chatview.h
        chatview.ui
        messagebubble.cpp
        messagebubble.h
        chatitembase.cpp
        chatitembase.h
        StateWidget.cpp
        StateWidget.h
        SearchList.cpp
        SearchList.h
        userdata.cpp
        userdata.h
        adduseritem.cpp
        adduseritem.h
        adduseritem.ui
        findsuccessdlg.cpp
        findsuccessdlg.h
        findsuccessdlg.ui
        applyfriend.cpp
        applyfriend.h
        applyfriend.ui
        ClickedOnceLabel.cpp
        ClickedOnceLabel.h
        friendlabel.cpp
        friendlabel.h
        friendlabel.ui
        conuseritem.cpp
        conuseritem.h
        conuseritem.ui
        ContactUserList.cpp
        ContactUserList.h
        grouptipitem.cpp
        grouptipitem.h
        grouptipitem.ui
        applyfriendpage.cpp
        applyfriendpage.h
        applyfriendpage.ui
        applyfrienditem.cpp
        applyfrienditem.h
        applyfrienditem.ui
        ApplyFriendList.cpp
        ApplyFriendList.h
        authenfriend.cpp
        authenfriend.h
        authenfriend.ui
        MessageTextEdit.cpp
        MessageTextEdit.h
        friendinfopage.cpp
        friendinfopage.h
        friendinfopage.ui
        TCPFileMgr.cpp
        TCPFileMgr.h
        userinfopage.cpp
        userinfopage.h
        userinfopage.ui
        ImageCropperDialog.cpp
        ImageCropperDialog.h
        ImageCropperLabel.cpp
        ImageCropperLabel.h
        PictureBubble.cpp
        PictureBubble.h
        FileCardWidget.cpp
        FileCardWidget.h
        FileBubble.cpp
        FileBubble.h
        CheckIndicator.h
        CheckIndicator.cpp
        GroupAvatarLabel.cpp
        GroupAvatarLabel.h
        applygroupchat.cpp
        applygroupchat.h
        applygroupchat.ui
        addgroupitem.cpp
        addgroupitem.h
        addgroupitem.ui
        chatfriendinfoshow.cpp
        chatfriendinfoshow.h
        chatfriendinfoshow.ui
        chatgroupinfoshow.cpp
        chatgroupinfoshow.h
        chatgroupinfoshow.ui
        SherpaOnnxRecognizer.cpp
        SherpaOnnxRecognizer.h
        AudioRecorder.cpp
        AudioRecorder.h
)

if(WIN32)
    set(SHERPA_ONNX_ROOT "${PROJECT_SOURCE_DIR}/thirdparty/sherpa-onnx-v1.13.8-win-x64-shared-MD-Release")
    add_library(sherpa-onnx-c-api SHARED IMPORTED)
    set_target_properties(sherpa-onnx-c-api PROPERTIES
            IMPORTED_LOCATION "${SHERPA_ONNX_ROOT}/lib/sherpa-onnx-c-api.dll"
            IMPORTED_IMPLIB "${SHERPA_ONNX_ROOT}/lib/sherpa-onnx-c-api.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${SHERPA_ONNX_ROOT}/include"
    )
    target_link_libraries(mptchat PRIVATE sherpa-onnx-c-api)

    add_custom_command(TARGET mptchat POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${SHERPA_ONNX_ROOT}/lib/sherpa-onnx-c-api.dll"
                    "${SHERPA_ONNX_ROOT}/bin/onnxruntime.dll"
                    "${SHERPA_ONNX_ROOT}/bin/onnxruntime_providers_shared.dll"
                    "$<TARGET_FILE_DIR:mptchat>")
endif()

target_link_libraries(mptchat
        PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::Multimedia
)
