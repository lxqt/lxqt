<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="el">
<context>
    <name>PasswordDialog</name>
    <message>
        <location filename="../passworddialog.ui" line="6"/>
        <location filename="../obj-x86_64-linux-gnu/lxqt-sudo_autogen/include/ui_passworddialog.h" line="171"/>
        <source>LXQt sudo</source>
        <translation>LXQt sudo</translation>
    </message>
    <message>
        <location filename="../passworddialog.ui" line="42"/>
        <location filename="../obj-x86_64-linux-gnu/lxqt-sudo_autogen/include/ui_passworddialog.h" line="173"/>
        <source>Copy command to clipboard</source>
        <translation>Αντιγραφή της εντολής στο πρόχειρο</translation>
    </message>
    <message>
        <location filename="../passworddialog.ui" line="45"/>
        <location filename="../obj-x86_64-linux-gnu/lxqt-sudo_autogen/include/ui_passworddialog.h" line="175"/>
        <source>&amp;Copy</source>
        <translation>&amp;Αντιγραφή</translation>
    </message>
    <message>
        <location filename="../passworddialog.ui" line="83"/>
        <location filename="../obj-x86_64-linux-gnu/lxqt-sudo_autogen/include/ui_passworddialog.h" line="176"/>
        <source>The requested action needs administrative privileges.&lt;br&gt;Please enter your password.</source>
        <translation>Η αιτηθείσα ενέργεια απαιτεί προνόμια διαχειριστή.&lt;br&gt;Παρακαλώ εισαγάγετε τον κωδικό πρόσβασης.</translation>
    </message>
    <message>
        <location filename="../passworddialog.ui" line="106"/>
        <location filename="../obj-x86_64-linux-gnu/lxqt-sudo_autogen/include/ui_passworddialog.h" line="178"/>
        <source>LXQt sudo backend</source>
        <translation>Σύστημα υποστήριξης sudo LXQt</translation>
    </message>
    <message>
        <location filename="../passworddialog.ui" line="109"/>
        <location filename="../obj-x86_64-linux-gnu/lxqt-sudo_autogen/include/ui_passworddialog.h" line="181"/>
        <source>A program LXQt sudo calls in background to elevate privileges.</source>
        <translation>Ένα πρόγραμμα που καλείται από το LXQt sudo για την παραχώρηση προνομίων.</translation>
    </message>
    <message>
        <location filename="../passworddialog.ui" line="119"/>
        <location filename="../obj-x86_64-linux-gnu/lxqt-sudo_autogen/include/ui_passworddialog.h" line="183"/>
        <source>Command:</source>
        <translation>Εντολή:</translation>
    </message>
    <message>
        <location filename="../passworddialog.ui" line="126"/>
        <location filename="../obj-x86_64-linux-gnu/lxqt-sudo_autogen/include/ui_passworddialog.h" line="184"/>
        <source>Password:</source>
        <translation>Κωδικός πρόσβασης:</translation>
    </message>
    <message>
        <location filename="../passworddialog.ui" line="133"/>
        <location filename="../obj-x86_64-linux-gnu/lxqt-sudo_autogen/include/ui_passworddialog.h" line="186"/>
        <source>Enter password</source>
        <translation>Εισαγάγετε τον κωδικό πρόσβασης</translation>
    </message>
    <message>
        <location filename="../passworddialog.cpp" line="60"/>
        <source>Attempt #%1</source>
        <translation>Προσπάθεια #%1</translation>
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
        <translation>Χρήση: %1 επιλογή [εντολή [ορίσματα...]]]

Γραφικό περιβάλλον για το %2/%3

Ορίσματα:
  επιλογή:
    -h|--help      Ενφάνιση της βοήθειας.
    -v|--version   Εωφάνιση της έκδοσης.
    -s|--su        Χρήση του %3(1) ως backend.
    -d|--sudo      Χρήση του %2(8) ως backend.
  εντολή           Εντολή προς εκτέλεση.
  ορίσματα         Προαιρετικά ορίσματα της εντολής.

</translation>
    </message>
    <message>
        <location filename="../sudo.cpp" line="86"/>
        <source>%1 version %2
</source>
        <translation>%1 έκδοση %2
</translation>
    </message>
</context>
<context>
    <name>Sudo</name>
    <message>
        <location filename="../sudo.cpp" line="189"/>
        <source>%1: no command to run provided!</source>
        <translation>%1: δε δόθηκε κάποια εντολή προς εκτέλεση!</translation>
    </message>
    <message>
        <location filename="../sudo.cpp" line="196"/>
        <source>%1: no backend chosen!</source>
        <translation>%1: δεν επιλέξατε το backend!</translation>
    </message>
    <message>
        <location filename="../sudo.cpp" line="213"/>
        <source>Syscall error, failed to fork: %1</source>
        <translation>Σφάλμα κλήσης συστήματος, αποτυχία δημιουργίας νέας διεργασίας: %1</translation>
    </message>
    <message>
        <location filename="../sudo.cpp" line="240"/>
        <source>unset</source>
        <extracomment>shouldn&apos;t be actually used but keep as short as possible in translations just in case.</extracomment>
        <translation>ανενεργό</translation>
    </message>
    <message>
        <location filename="../sudo.cpp" line="289"/>
        <source>%1: Detected attempt to inject privileged command via LC_ALL env(%2). Exiting!
</source>
        <translation>%1: Εντοπίστηκε απόπειρα έγχυσης προνομιούχας εντολής μέσω του LC_ALL env(%2). Εγκατάλειψη!
</translation>
    </message>
    <message>
        <location filename="../sudo.cpp" line="331"/>
        <source>Syscall error, failed to bring pty to non-block mode: %1</source>
        <translation>Σφάλμα κλήσης συστήματος, αποτυχία διάθεσης του pty σε non-blocking λειτουργία: %1</translation>
    </message>
    <message>
        <location filename="../sudo.cpp" line="339"/>
        <source>Syscall error, failed to fdopen pty: %1</source>
        <translation>Σφάλμα κλήσης συστήματος, αποτυχία κλήσης της fdopen για το pty: %1</translation>
    </message>
    <message>
        <location filename="../sudo.cpp" line="308"/>
        <source>%1: Failed to exec &apos;%2&apos;: %3
</source>
        <translation>%1: Αποτυχία εκτέλεσης του «%2»: «%3»
</translation>
    </message>
    <message>
        <location filename="../sudo.cpp" line="370"/>
        <source>Child &apos;%1&apos; process failed!
%2</source>
        <translation>Η διεργασία παιδί «%1» απέτυχε!
%2</translation>
    </message>
</context>
</TS>
