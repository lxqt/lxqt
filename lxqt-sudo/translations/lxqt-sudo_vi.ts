<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="vi">
<context>
    <name>PasswordDialog</name>
    <message>
        <location filename="../passworddialog.ui" line="6"/>
        <location filename="../obj-x86_64-linux-gnu/lxqt-sudo_autogen/include/ui_passworddialog.h" line="171"/>
        <source>LXQt sudo</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../passworddialog.ui" line="42"/>
        <location filename="../obj-x86_64-linux-gnu/lxqt-sudo_autogen/include/ui_passworddialog.h" line="173"/>
        <source>Copy command to clipboard</source>
        <translation>Chép câu lệnh vào bảng nhớ tạm</translation>
    </message>
    <message>
        <location filename="../passworddialog.ui" line="45"/>
        <location filename="../obj-x86_64-linux-gnu/lxqt-sudo_autogen/include/ui_passworddialog.h" line="175"/>
        <source>&amp;Copy</source>
        <translation>&amp;Sao chép</translation>
    </message>
    <message>
        <location filename="../passworddialog.ui" line="83"/>
        <location filename="../obj-x86_64-linux-gnu/lxqt-sudo_autogen/include/ui_passworddialog.h" line="176"/>
        <source>The requested action needs administrative privileges.&lt;br&gt;Please enter your password.</source>
        <translation>Thực hiện hành động này yêu cầu quyền quản trị.&lt;br&gt;Vui lòng nhập mật khẩu.</translation>
    </message>
    <message>
        <location filename="../passworddialog.ui" line="106"/>
        <location filename="../obj-x86_64-linux-gnu/lxqt-sudo_autogen/include/ui_passworddialog.h" line="178"/>
        <source>LXQt sudo backend</source>
        <translation>Phần phụ trợ cho LXQt sudo</translation>
    </message>
    <message>
        <location filename="../passworddialog.ui" line="109"/>
        <location filename="../obj-x86_64-linux-gnu/lxqt-sudo_autogen/include/ui_passworddialog.h" line="181"/>
        <source>A program LXQt sudo calls in background to elevate privileges.</source>
        <translation>Một chương trình LXQt gọi sudo ở nền để đánh giá quyền hạn.</translation>
    </message>
    <message>
        <location filename="../passworddialog.ui" line="119"/>
        <location filename="../obj-x86_64-linux-gnu/lxqt-sudo_autogen/include/ui_passworddialog.h" line="183"/>
        <source>Command:</source>
        <translation>Lệnh:</translation>
    </message>
    <message>
        <location filename="../passworddialog.ui" line="126"/>
        <location filename="../obj-x86_64-linux-gnu/lxqt-sudo_autogen/include/ui_passworddialog.h" line="184"/>
        <source>Password:</source>
        <translation>Mật khẩu:</translation>
    </message>
    <message>
        <location filename="../passworddialog.ui" line="133"/>
        <location filename="../obj-x86_64-linux-gnu/lxqt-sudo_autogen/include/ui_passworddialog.h" line="186"/>
        <source>Enter password</source>
        <translation>Nhập mật khẩu</translation>
    </message>
    <message>
        <location filename="../passworddialog.cpp" line="60"/>
        <source>Attempt #%1</source>
        <translation>Lần thử thứ #%1</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../sudo.cpp" line="69"/>
        <source>Usage: %1 option [command [arguments...]]

GUI frontend for %2/%3

Arguments:
  option:
    -h|--help      Print this help.
    -v|--version   Print version information.
    -s|--su        Use %3(1) as backend.
    -d|--sudo      Use %2(8) as backend.
  command          Command to run.
  arguments        Optional arguments for command.

</source>
        <translation>Cách dùng: %1 tùy chọn [lệnh [đối mục...]]

Giao diện đồ hoạ cho %2/%3

Các đối mục:
  tùy chọn:
    -h|--help      In ra hướng dẫn này.
    -v|--version   In ra thông tin phiên bản
    -s|--su        Sử dụng %3(1) cho việc chạy lệnh.
    -d|--sudo      Sử dụng %2(8) cho việc chạy lệnh.
  lệnh          Câu lệnh để thực hiện.
  đối mục        Các đối mục cho câu lệnh. Có thể có hoặc không.

</translation>
    </message>
    <message>
        <location filename="../sudo.cpp" line="86"/>
        <source>%1 version %2
</source>
        <translation>%1 phiên bản %2
</translation>
    </message>
</context>
<context>
    <name>Sudo</name>
    <message>
        <location filename="../sudo.cpp" line="189"/>
        <source>%1: no command to run provided!</source>
        <translation>%1: không có lệnh nào để chạy!</translation>
    </message>
    <message>
        <location filename="../sudo.cpp" line="196"/>
        <source>%1: no backend chosen!</source>
        <translation>%1: không có chương trình phụ trợ (backend) nào được chọn!</translation>
    </message>
    <message>
        <location filename="../sudo.cpp" line="213"/>
        <source>Syscall error, failed to fork: %1</source>
        <translation>Lỗi thực thi lời gọi hệ thống, chia tiến trình thất bại: %1</translation>
    </message>
    <message>
        <location filename="../sudo.cpp" line="240"/>
        <source>unset</source>
        <extracomment>shouldn&apos;t be actually used but keep as short as possible in translations just in case.</extracomment>
        <translation>bỏ chọn</translation>
    </message>
    <message>
        <location filename="../sudo.cpp" line="289"/>
        <source>%1: Detected attempt to inject privileged command via LC_ALL env(%2). Exiting!
</source>
        <translation>%1: Đã phát hiện chèn lệnh với quyền quản trị qua biến môi trường LC_ALL (%2). Chấm dứt thực thi!
</translation>
    </message>
    <message>
        <location filename="../sudo.cpp" line="331"/>
        <source>Syscall error, failed to bring pty to non-block mode: %1</source>
        <translation>Lỗi thực thi lời gọi hệ thống, thất bại khi chuyển pty sang chế độ không chặn luồng thực thi: %1</translation>
    </message>
    <message>
        <location filename="../sudo.cpp" line="339"/>
        <source>Syscall error, failed to fdopen pty: %1</source>
        <translation>Lỗi thực thi lời gọi hệ thống, thất bại khi gọi hàm fdopen() trên pty: %1</translation>
    </message>
    <message>
        <location filename="../sudo.cpp" line="308"/>
        <source>%1: Failed to exec &apos;%2&apos;: %3
</source>
        <translation>%1: Thất bại khi cố gắng thực thi &apos;%2&apos;: %3
</translation>
    </message>
    <message>
        <location filename="../sudo.cpp" line="370"/>
        <source>Child &apos;%1&apos; process failed!
%2</source>
        <translation>Tiến trình con &apos;%1&apos; failed!
%2</translation>
    </message>
</context>
</TS>
